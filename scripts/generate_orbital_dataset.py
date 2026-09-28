#!/usr/bin/env python3
"""
scripts/generate_orbital_dataset.py
-----------------------------------
AstroGenesis Orbital Stability Training Dataset Generator (Vectorized & High Performance)

Simulates randomized multi-planet systems using symplectic integration with vectorized
NumPy operations. Detects physical instability (orbit crossing, ejections, close encounters)
and records features at t=0 for training the ML model.
"""

import math
import random
import os
import sys
import numpy as np
import pandas as pd
import time

# Ensure unbuffered stdout for live progress logging
sys.stdout.reconfigure(line_buffering=True)

# Physical constants in SI units (matching AstroGenesis PhysicsEngine)
G_CONST = 6.67430e-11       # m^3 kg^-1 s^-2
AU_METERS = 149597870700.0  # 1 AU in meters
SOLAR_MASS_KG = 1.9885e30   # M_Sun
EARTH_MASS_KG = 5.97219e24  # M_Earth

def compute_amd(masses, a_arr, e_arr, m_star):
    """
    Computes normalized Angular Momentum Deficit (AMD) as formulated by Laskar & Petit.
    AMD = sum( m_i * sqrt(G*M_*a_i) * (1 - sqrt(1 - e_i^2)) ) / sum( m_i * sqrt(G*M_*a_i) )
    """
    num = 0.0
    den = 0.0
    for m, a, e in zip(masses, a_arr, e_arr):
        if e >= 1.0:
            return 1.0
        lambda_circ = m * math.sqrt(G_CONST * m_star * a)
        num += lambda_circ * (1.0 - math.sqrt(max(0.0, 1.0 - e * e)))
        den += lambda_circ
    return (num / den) if den > 0.0 else 0.0

def simulate_system_vectorized(m_star, planet_masses, a_au, e_arr, max_orbits=600, dt_fraction=0.04):
    """
    Simulates multi-planet system using vectorized Leapfrog / Velocity Verlet.
    Returns: (is_stable: int [1 or 0], reason: str, steps_run: int)
    """
    n_planets = len(planet_masses)
    all_masses = np.array([m_star] + planet_masses, dtype=np.float64)
    n_bodies = n_planets + 1

    varpi = np.random.uniform(0, 2 * math.pi, n_planets)
    M_anom = np.random.uniform(0, 2 * math.pi, n_planets)

    positions = np.zeros((n_bodies, 2), dtype=np.float64)
    velocities = np.zeros((n_bodies, 2), dtype=np.float64)

    a_meters = [a * AU_METERS for a in a_au]

    for i in range(n_planets):
        a_m = a_meters[i]
        e = e_arr[i]
        mu = G_CONST * (m_star + planet_masses[i])

        # Kepler equation solve
        E = M_anom[i]
        for _ in range(5):
            E -= (E - e * math.sin(E) - M_anom[i]) / (1.0 - e * math.cos(E))

        cos_nu = (math.cos(E) - e) / (1.0 - e * math.cos(E))
        sin_nu = (math.sqrt(max(0.0, 1.0 - e * e)) * math.sin(E)) / (1.0 - e * math.cos(E))
        nu = math.atan2(sin_nu, cos_nu)

        r = a_m * (1.0 - e * e) / (1.0 + e * math.cos(nu))
        h = math.sqrt(mu * a_m * max(0.0, 1.0 - e * e))

        v_r = (mu / h) * e * math.sin(nu)
        v_t = (mu / h) * (1.0 + e * math.cos(nu))

        phi = nu + varpi[i]
        cos_phi = math.cos(phi)
        sin_phi = math.sin(phi)

        positions[i + 1, 0] = r * cos_phi
        positions[i + 1, 1] = r * sin_phi
        velocities[i + 1, 0] = v_r * cos_phi - v_t * sin_phi
        velocities[i + 1, 1] = v_r * sin_phi + v_t * cos_phi

    # Shift to Barycentric Frame
    total_mass = np.sum(all_masses)
    com_pos = np.sum(positions * all_masses[:, None], axis=0) / total_mass
    com_vel = np.sum(velocities * all_masses[:, None], axis=0) / total_mass
    positions -= com_pos
    velocities -= com_vel

    mass_row = all_masses[None, :] # shape (1, N)

    def compute_acc_vec(pos):
        # diff[i, j] = pos[j] - pos[i]
        diff = pos[None, :, :] - pos[:, None, :] # shape (N, N, 2)
        dist_sq = np.sum(diff**2, axis=-1)       # shape (N, N)
        np.fill_diagonal(dist_sq, 1e12)          # avoid div by zero on diagonal

        inv_dist3 = dist_sq ** (-1.5)
        np.fill_diagonal(inv_dist3, 0.0)

        # weights[i, j] = mass_j * inv_dist3[i, j]
        weights = mass_row * inv_dist3           # shape (N, N)
        return G_CONST * np.sum(diff * weights[:, :, None], axis=1)

    p_inner = 2.0 * math.pi * math.sqrt(a_meters[0]**3 / (G_CONST * m_star))
    dt = dt_fraction * p_inner
    max_steps = int(max_orbits / dt_fraction) # e.g. 600 / 0.04 = 15,000 steps

    acc = compute_acc_vec(positions)

    # Simulation loop
    for step in range(max_steps):
        positions += velocities * dt + 0.5 * acc * (dt * dt)
        new_acc = compute_acc_vec(positions)
        velocities += 0.5 * (acc + new_acc) * dt
        acc = new_acc

        # Stability checks every 30 steps
        if step % 30 == 0 and step > 0:
            # Check positions relative to star (body 0)
            star_pos = positions[0]
            rel_pos = positions[1:] - star_pos
            r_dists = np.sqrt(np.sum(rel_pos**2, axis=1))

            # 1. Ejections or stellar fall
            if np.any(r_dists > 50.0 * AU_METERS):
                return 0, "Ejection (escaped to >50 AU)", step
            if np.any(r_dists < 0.03 * AU_METERS):
                return 0, "Stellar collision", step

            # 2. Distance sorting / orbit crossings
            for k in range(n_planets - 1):
                if r_dists[k] > r_dists[k + 1] * 1.05:
                    return 0, "Orbit crossing detected", step

            # 3. Pairwise close encounters
            for i in range(1, n_bodies):
                for j in range(i + 1, n_bodies):
                    dist_ij = np.linalg.norm(positions[i] - positions[j])
                    r_hill = ((all_masses[i] + all_masses[j]) / (3.0 * m_star))**(1.0 / 3.0) * r_dists[i-1]
                    if dist_ij < 0.15 * r_hill:
                        return 0, "Close encounter / collision", step

    return 1, "Stable throughout simulation", max_steps

def generate_system_features():
    m_star = random.uniform(0.4, 1.8) * SOLAR_MASS_KG
    n_planets = random.randint(2, 5)
    a1 = 10.0 ** random.uniform(-0.8, 0.2) # 0.16 to 1.58 AU

    planet_masses = []
    for _ in range(n_planets):
        if random.random() < 0.70:
            m = 10.0 ** random.uniform(-0.7, 1.2) * EARTH_MASS_KG
        else:
            m = 10.0 ** random.uniform(1.2, 2.8) * EARTH_MASS_KG
        planet_masses.append(m)

    spacing_regime = random.random()
    if spacing_regime < 0.38:
        base_delta = random.uniform(1.5, 4.2)  # Tight / chaotic
    elif spacing_regime < 0.72:
        base_delta = random.uniform(4.2, 8.5)  # Moderate / borderline
    else:
        base_delta = random.uniform(8.5, 22.0) # Wide / stable

    a_au = [a1]
    for i in range(1, n_planets):
        delta_ij = max(1.2, base_delta * random.uniform(0.85, 1.25))
        m_sum = planet_masses[i - 1] + planet_masses[i]
        mu_hill = (m_sum / (3.0 * m_star)) ** (1.0 / 3.0)
        h = 0.5 * delta_ij * mu_hill
        if h >= 0.9:
            h = 0.85
        a_next = a_au[-1] * (1.0 + h) / (1.0 - h)
        a_au.append(a_next)

    ecc_regime = random.random()
    e_arr = []
    for _ in range(n_planets):
        if ecc_regime < 0.50:
            e = abs(random.gauss(0.02, 0.02))
        elif ecc_regime < 0.80:
            e = random.uniform(0.08, 0.25)
        else:
            e = random.uniform(0.25, 0.55)
        e_arr.append(min(0.7, max(0.001, e)))

    total_planet_mass = sum(planet_masses)
    total_mass_ratio = total_planet_mass / m_star
    max_mass_ratio = max(planet_masses) / m_star

    mutual_deltas = []
    has_orbit_crossing = 0
    for i in range(n_planets - 1):
        m_sum = planet_masses[i] + planet_masses[i + 1]
        a_mean = 0.5 * (a_au[i] + a_au[i + 1])
        r_hill = ((m_sum / (3.0 * m_star)) ** (1.0 / 3.0)) * a_mean
        delta = (a_au[i + 1] - a_au[i]) / r_hill
        mutual_deltas.append(delta)

        r_apo_inner = a_au[i] * (1.0 + e_arr[i])
        r_peri_outer = a_au[i + 1] * (1.0 - e_arr[i + 1])
        if r_apo_inner >= r_peri_outer:
            has_orbit_crossing = 1

    min_mutual_hill_sep = min(mutual_deltas)
    mean_mutual_hill_sep = sum(mutual_deltas) / len(mutual_deltas)
    period_ratios = [(a_au[i + 1] / a_au[i]) ** 1.5 for i in range(n_planets - 1)]
    min_period_ratio = min(period_ratios)
    max_eccentricity = max(e_arr)
    mean_eccentricity = sum(e_arr) / len(e_arr)
    amd = compute_amd(planet_masses, [a * AU_METERS for a in a_au], e_arr, m_star)

    features = {
        "body_count": n_planets,
        "star_mass_solar": m_star / SOLAR_MASS_KG,
        "total_mass_ratio": total_mass_ratio,
        "max_mass_ratio": max_mass_ratio,
        "min_mutual_hill_sep": min_mutual_hill_sep,
        "mean_mutual_hill_sep": mean_mutual_hill_sep,
        "max_eccentricity": max_eccentricity,
        "mean_eccentricity": mean_eccentricity,
        "orbit_crossing_flag": has_orbit_crossing,
        "min_period_ratio": min_period_ratio,
        "angular_momentum_deficit": amd,
        "energy_drift_pct": 0.0
    }

    return m_star, planet_masses, a_au, e_arr, features

def main():
    print("==========================================================", flush=True)
    print(" AstroGenesis Orbital Stability Dataset Generator (Vectorized)", flush=True)
    print("==========================================================", flush=True)

    out_dir = "data"
    os.makedirs(out_dir, exist_ok=True)
    out_csv = os.path.join(out_dir, "orbital_dataset.csv")

    TARGET_SAMPLES = 2000
    print(f"Targeting {TARGET_SAMPLES} simulated configurations...", flush=True)

    rows = []
    stable_count = 0
    unstable_count = 0
    t0 = time.time()

    np.random.seed(42)
    random.seed(42)

    for idx in range(TARGET_SAMPLES):
        m_star, planet_masses, a_au, e_arr, feat = generate_system_features()

        is_stable, reason, steps = simulate_system_vectorized(
            m_star, planet_masses, a_au, e_arr, max_orbits=400, dt_fraction=0.04
        )

        feat["is_stable"] = is_stable
        rows.append(feat)

        if is_stable == 1:
            stable_count += 1
        else:
            unstable_count += 1

        if (idx + 1) % 100 == 0 or idx == TARGET_SAMPLES - 1:
            elapsed = time.time() - t0
            rate = (idx + 1) / elapsed
            print(f"  [{idx + 1:4d}/{TARGET_SAMPLES}] Stable: {stable_count:4d} | Unstable: {unstable_count:4d} | Rate: {rate:.1f} sys/s", flush=True)

    df = pd.DataFrame(rows)
    df.to_csv(out_csv, index=False)
    print(f"\n[Dataset] Successfully saved {len(df)} samples to {out_csv}", flush=True)
    print(f"  - Stable:   {stable_count} ({stable_count/len(df)*100:.1f}%)", flush=True)
    print(f"  - Unstable: {unstable_count} ({unstable_count/len(df)*100:.1f}%)", flush=True)

if __name__ == "__main__":
    main()
