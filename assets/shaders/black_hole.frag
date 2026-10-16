#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoord;
in vec3 RayDir;

// Spacetime Geometry & Physical Metrics
uniform vec3 uCameraPos;
uniform vec3 uBhCenter;
uniform float uBoundRadius;
uniform float uSchwRadius;      // r_s = 2GM/c^2
uniform float uHorizonRadius;   // r_+
uniform float uPhotonRadius;    // r_ph
uniform float uShadowRadius;    // b_crit = sqrt(27)/2 * r_s
uniform float uSpinParam;       // a* in [-1.0, 1.0]
uniform vec3 uSpinAxis;         // Normalized spin axis vector

// Accretion Disk Physics (Novikov-Thorne)
uniform bool uHasDisk;
uniform float uDiskInnerRadius; // r_in = r_isco
uniform float uDiskOuterRadius; // r_out
uniform float uDiskPeakTemp;    // T_peak in Kelvin (live from simulation)
uniform float uDiskLuminosity;  // Total disk luminosity in Watts
uniform float uAccretionRate;   // Mdot in M_sun/yr (live from simulation)
uniform float uSimTime;

// General-Purpose Gravitational Lensing of Active Celestial Bodies
const int MAX_LENSED_BODIES = 16;
uniform int uNumLensedBodies;
uniform vec3 uLensedBodyPos[MAX_LENSED_BODIES];       // Relative to BH center in AU
uniform float uLensedBodyRadius[MAX_LENSED_BODIES];   // in AU
uniform vec3 uLensedBodyColor[MAX_LENSED_BODIES];     // Base / spectral color
uniform float uLensedBodyTemp[MAX_LENSED_BODIES];     // Surface temp in Kelvin
uniform float uLensedBodyLum[MAX_LENSED_BODIES];      // Relative luminosity factor
uniform int uLensedBodyIsStar[MAX_LENSED_BODIES];     // 1: star, 0: planet/moon

// Visual Mode & Background Skybox
uniform int uVisualMode;        // 0: Normal, 1: Realistic, 2: Cinematic
uniform sampler2D uSkyboxTex;
uniform bool uHasSkybox;

const float PI = 3.14159265358979323846;
const float TWO_PI = 6.28318530717958647692;

// ── Hash & Procedural 3D Noise for Disk Keplerian Turbulence ─────────────────
float hash31(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.1, 0.1));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);

    float n000 = hash31(i + vec3(0.0, 0.0, 0.0));
    float n100 = hash31(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash31(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash31(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash31(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash31(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash31(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash31(i + vec3(1.0, 1.0, 1.0));

    float x00 = mix(n000, n100, u.x);
    float x10 = mix(n010, n110, u.x);
    float x01 = mix(n001, n101, u.x);
    float x11 = mix(n011, n111, u.x);

    float y0 = mix(x00, x10, u.y);
    float y1 = mix(x01, x11, u.y);

    return mix(y0, y1, u.z);
}

float fbmDisk(vec3 p) {
    float v = 0.0;
    float a = 0.55;
    vec3 shift = vec3(100.0);
    for (int i = 0; i < 3; ++i) {
        v += a * noise3D(p);
        p = p * 2.2 + shift;
        a *= 0.48;
    }
    return v;
}

// ── Physical Chromaticity Palette: Fiery Ember to Electric Diamond Cyan ───────
vec3 temperatureToColor(float T_kelvin) {
    float T = clamp(T_kelvin, 800.0, 36000.0);

    vec3 col;
    if (T < 2400.0) {
        // Deep smoldering ruby red to dark ember
        float t = (T - 800.0) / 1600.0;
        col = mix(vec3(0.60, 0.04, 0.01), vec3(0.95, 0.22, 0.02), t);
    } else if (T < 4500.0) {
        // Fiery amber to rich golden flame
        float t = (T - 2400.0) / 2100.0;
        col = mix(vec3(0.95, 0.22, 0.02), vec3(1.00, 0.65, 0.10), t);
    } else if (T < 7800.0) {
        // Incandescent solar gold to pure white
        float t = (T - 4500.0) / 3300.0;
        col = mix(vec3(1.00, 0.65, 0.10), vec3(1.00, 0.96, 0.90), t);
    } else if (T < 15000.0) {
        // Radiant stellar white to crisp electric diamond cyan
        float t = (T - 7800.0) / 7200.0;
        col = mix(vec3(1.00, 0.96, 0.90), vec3(0.68, 0.88, 1.20), t);
    } else {
        // High-energy extreme relativistic ultraviolet-blue
        float t = clamp((T - 15000.0) / 21000.0, 0.0, 1.0);
        col = mix(vec3(0.68, 0.88, 1.20), vec3(0.48, 0.72, 1.40), t);
    }

    return col;
}

// ── Convert 3D Direction to Equirectangular Skybox UV Coordinates ─────────────
vec2 dirToSkyboxUV(vec3 dir) {
    vec3 d = normalize(dir);
    float u = fract(1.0 - atan(d.z, d.x) / TWO_PI);
    float v = clamp(0.5 - asin(clamp(d.y, -1.0, 1.0)) / PI, 0.001, 0.999);
    return vec2(u, v);
}

// ── Background Skybox Sampling with Radiometric Linearization ────────────────
vec3 sampleSkyboxColor(vec3 dir) {
    if (!uHasSkybox) return vec3(0.0);
    vec2 uv = dirToSkyboxUV(dir);
    vec3 raw = texture(uSkyboxTex, uv).rgb;
    return pow(raw, vec3(2.2)) * 1.5;
}

// ── Ray-Sphere Intersection Helper for Celestial Bodies ──────────────────────
bool intersectSphere(vec3 ro, vec3 rd, vec3 center, float radius, out float tHit, out vec3 normal) {
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0) return false;
    float sqrtDisc = sqrt(disc);
    float t0 = -b - sqrtDisc;
    float t1 = -b + sqrtDisc;
    if (t0 > 1e-4) {
        tHit = t0;
        normal = normalize(oc + rd * t0);
        return true;
    } else if (t1 > 1e-4) {
        tHit = t1;
        normal = normalize(oc + rd * t1);
        return true;
    }
    return false;
}

// ── General-Purpose Sampling of Lensed Celestial Bodies ──────────────────────
// Intersects rays with individual stars and planets, including stellar limb darkening
// and luminous atmospheric coronas for authentic Einstein rings and arcs.
vec3 sampleLensedBodies(vec3 rayOrig, vec3 rayDir, float maxDist) {
    vec3 totalRadiance = vec3(0.0);
    float closestT = maxDist;
    int hitIdx = -1;
    vec3 hitNorm = vec3(0.0);

    // 1. Direct surface intersection
    for (int i = 0; i < uNumLensedBodies && i < MAX_LENSED_BODIES; ++i) {
        float t;
        vec3 norm;
        if (intersectSphere(rayOrig, rayDir, uLensedBodyPos[i], uLensedBodyRadius[i], t, norm)) {
            if (t < closestT) {
                closestT = t;
                hitIdx = i;
                hitNorm = norm;
            }
        }
    }

    if (hitIdx >= 0) {
        // Direct hit on body surface
        vec3 col = uLensedBodyColor[hitIdx];
        if (uLensedBodyIsStar[hitIdx] == 1) {
            // Star surface: Planck blackbody with Eddington limb darkening
            float mu = clamp(dot(hitNorm, -rayDir), 0.0, 1.0);
            float limbDarkening = (1.0 + 2.0 * mu) / 3.0;
            totalRadiance = col * uLensedBodyLum[hitIdx] * limbDarkening * 3.5;
        } else {
            // Planet surface
            float diff = max(0.0, dot(hitNorm, -rayDir));
            totalRadiance = col * (0.2 + 0.8 * diff);
        }
        return totalRadiance;
    }

    // 2. Extended Luminous Corona / Diffraction Glare for Stars (Produces authentic Einstein Rings & Arcs)
    for (int i = 0; i < uNumLensedBodies && i < MAX_LENSED_BODIES; ++i) {
        if (uLensedBodyIsStar[i] == 1) {
            vec3 toStar = uLensedBodyPos[i] - rayOrig;
            float tProj = dot(toStar, rayDir);
            if (tProj > 0.0) {
                vec3 closestPt = rayOrig + rayDir * tProj;
                float dPerpSq = dot(closestPt - uLensedBodyPos[i], closestPt - uLensedBodyPos[i]);
                float rStar = uLensedBodyRadius[i];
                float rStarSq = rStar * rStar;

                // Two-component stellar glare: intense inner core + wide diffraction halo
                float core = rStarSq / (dPerpSq + 0.08 * rStarSq);
                core = pow(core, 1.6) * 1.8;

                float rHalo = rStar * 1.8;
                float rHaloSq = rHalo * rHalo;
                float halo = rHaloSq / (dPerpSq + 0.25 * rHaloSq);
                halo = pow(halo, 1.4) * 0.6;

                float rMax = rStar * 4.5;
                float dPerp = sqrt(dPerpSq);
                float fade = clamp(1.0 - smoothstep(rStar * 1.2, rMax, dPerp), 0.0, 1.0);

                float totalGlare = (core + halo) * uLensedBodyLum[i] * fade;
                totalRadiance += uLensedBodyColor[i] * totalGlare;
            }
        }
    }

    return totalRadiance;
}

// ── Relativistic Geodesic Acceleration: a = d^2 x / d lambda^2 ────────────────
vec3 computeGeodesicAcceleration(vec3 pos, vec3 dir, float rs, float spinParam, vec3 spinNorm) {
    float rSq = dot(pos, pos);
    float r = sqrt(rSq);
    float r3 = rSq * r;
    float r5 = r3 * rSq;

    // Exact Schwarzschild spatial acceleration: a = -1.5 * rs * (h^2 / r^5) * r
    vec3 hVec = cross(pos, dir);
    float hSq = dot(hVec, hVec);
    vec3 aSchw = -1.5 * rs * (hSq / max(r5, 1e-10)) * pos;

    // Kerr Gravitomagnetic Frame-Dragging Acceleration:
    // B_GM = (rs^2 * a* / (4 * r^3)) * [ 3(S_hat . r_hat) * r_hat - S_hat ]
    // a_Kerr = 2 * (v x B_GM)
    vec3 aKerr = vec3(0.0);
    if (abs(spinParam) > 0.001 && r > 1e-4) {
        vec3 rNorm = pos / r;
        float sDotR = dot(spinNorm, rNorm);
        vec3 bGm = (0.25 * rs * rs * spinParam / max(r3, 1e-8)) * (3.0 * sDotR * rNorm - spinNorm);
        aKerr = 2.0 * cross(dir, bGm);
    }

    return aSchw + aKerr;
}

// ── Evaluate Accretion Disk Emission at a Given Point in the Disk Plane ───────
void sampleDiskAtPoint(
    vec3 hitPos,
    vec3 rayDir,
    float rs,
    float rIn,
    float rOut,
    vec3 spinNorm,
    inout vec3 diskRadiance,
    inout float diskAlpha
) {
    float r = length(hitPos);
    if (r < rIn * 0.98 || r > rOut * 1.02) return;

    float rNorm = r / rIn;
    float flare = sqrt(max(0.0, 1.0 - 1.0 / sqrt(rNorm)));
    // Astrophysical scale height: very thin near ISCO, gently flaring outward
    float diskH = r * (0.015 * flare + 0.0035);

    // Path length through disk inclined at angle theta_inc: ds = 2H / |cos(theta_inc)|
    float cosInc = abs(dot(rayDir, spinNorm));
    float dsPath = (2.0 * diskH) / max(cosInc, 0.035);

    // Astrophysical Novikov-Thorne Temperature Profile Driven by Live Physics Simulation:
    // Uses uDiskPeakTemp dynamically calculated from M and Mdot
    float fPeak = 0.4886; // value at rNorm = 1.778
    float fR = pow(1.0 / (rNorm * rNorm * rNorm), 0.25) * pow(max(0.0, 1.0 - 1.0 / sqrt(rNorm)), 0.25);
    float tNorm = clamp(fR / fPeak, 0.0, 1.0);
    float peakTemp = (uDiskPeakTemp > 100.0) ? uDiskPeakTemp : 14000.0;
    float localTemp = mix(1200.0, peakTemp, tNorm);

    // Relativistic Keplerian Orbital Motion in Kerr Spacetime
    float rg = rs * 0.5;
    float vBeta = clamp(sqrt(rg / r) / (1.0 + uSpinParam * pow(rg / r, 1.5)), 0.0, 0.85);
    vec3 vDir = normalize(cross(spinNorm, hitPos));

    // Relativistic Doppler Factor: delta = 1 / (gamma * (1 - beta . n))
    vec3 toObs = -rayDir;
    float betaDotN = clamp(vBeta * dot(vDir, toObs), -0.85, 0.85);
    float gamma = 1.0 / sqrt(1.0 - vBeta * vBeta);
    float dopplerFactor = 1.0 / (gamma * (1.0 - betaDotN));

    // Gravitational Redshift: g_grav = sqrt(1 - rs / r)
    float gGrav = sqrt(clamp(1.0 - rs / r, 0.01, 1.0));
    float totalShift = dopplerFactor * gGrav;

    // Relativistic Beaming: Bolometric Flux scales as g^4
    float beaming = clamp(pow(totalShift, 4.0), 0.02, 18.0);

    // Observed blackbody color at shifted temperature
    float observedTemp = localTemp * totalShift;
    vec3 emitCol = temperatureToColor(observedTemp);

    // Procedural Differential Keplerian Sheared Turbulence (Seamless: no branch cut)
    vec3 refX = normalize(abs(spinNorm.y) < 0.9 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0));
    vec3 refY = cross(spinNorm, refX);
    float phi = atan(dot(hitPos, refY), dot(hitPos, refX));

    float omegaKepler = pow(rIn / r, 1.5);
    float phiSheared = phi - omegaKepler * (uSimTime * 0.6 + 4.5 * log(rNorm));

    // Multi-octave azimuthal Keplerian shearing noise
    vec3 noiseCoord = vec3(cos(phiSheared) * 3.5, sin(phiSheared) * 3.5, rNorm * 1.5);
    float turb1 = fbmDisk(noiseCoord);
    vec3 fineCoord = vec3(cos(phiSheared * 2.2) * 8.0, sin(phiSheared * 2.2) * 8.0, rNorm * 3.8);
    float turb2 = fbmDisk(fineCoord);
    float turbulence = 0.40 + 0.35 * turb1 + 0.25 * turb2;

    // High-frequency Keplerian concentric shear striations
    float striations = 0.85 + 0.15 * sin(rNorm * 28.0 + 2.5 * turb1);

    // Harmonic multi-arm logarithmic spiral density waves
    float spiral1 = cos(2.0 * phiSheared - 3.0 * log(rNorm));
    float spiral2 = cos(4.0 * phiSheared - 5.5 * log(rNorm));
    float spiralArm = 0.70 + 0.22 * spiral1 + 0.08 * spiral2;

    // Dark cooler dust absorption lanes weaving through plasma
    float dustCoord = fbmDisk(vec3(cos(phiSheared * 1.2) * 5.0, sin(phiSheared * 1.2) * 5.0, rNorm * 2.0));
    float dustLane = smoothstep(0.15, 0.75, dustCoord);

    // Sharp ISCO inner edge truncation and gentle outer rim fade
    float innerCut = smoothstep(rIn * 0.985, rIn * 1.025, r);
    float outerCut = smoothstep(rOut * 1.01, rOut * 0.65, r);
    float density = turbulence * striations * spiralArm * (0.65 + 0.35 * dustLane) * innerCut * outerCut;

    // Radiative transfer optical depth
    float opacityCoeff = 2.8 / max(rs, 1e-4);
    float opticalDepth = density * dsPath * opacityCoeff;
    float alphaHit = clamp(1.0 - exp(-opticalDepth), 0.0, 0.95);

    // Radiance intensity dynamically driven by live accretion rate and disk luminosity
    float mdotFactor = (uAccretionRate > 1e-7) ? clamp(uAccretionRate / 1.0e-5, 0.2, 50.0) : 
                       ((uDiskLuminosity > 1e20) ? clamp(uDiskLuminosity / 5.7e31, 0.2, 50.0) : 1.0);

    // Relativistic ISCO plunge thermal emission enhancement
    float iscoPlunge = exp(-4.0 * max(0.0, rNorm - 1.0));
    vec3 plungeColor = vec3(0.95, 1.15, 1.50) * iscoPlunge * 1.6;

    vec3 hitRadiance = (emitCol * (0.75 + 0.45 * turbulence) + plungeColor) * beaming * mdotFactor * 1.8;

    diskRadiance += (1.0 - diskAlpha) * hitRadiance * alphaHit;
    diskAlpha += (1.0 - diskAlpha) * alphaHit;
}

void main() {
    vec3 camRel = uCameraPos - uBhCenter;
    float camDist = length(camRel);
    vec3 rayDir = normalize(RayDir);

    float rs = max(uSchwRadius, 1e-5);
    float rHorizon = max(uHorizonRadius, rs * 0.5);
    float rBound = max(uBoundRadius, rs * 6.0);
    float rIn = max(uDiskInnerRadius, rHorizon * 1.02);
    float rOut = max(uDiskOuterRadius, rIn * 1.5);

    vec3 spinNorm = length(uSpinAxis) > 0.01 ? normalize(uSpinAxis) : vec3(0.0, 1.0, 0.0);

    // ── Pre-Check: Foreground Celestial Bodies (Closer than Black Hole) ───────────
    float fgClosestT = camDist;
    int fgHitIdx = -1;
    vec3 fgHitNorm = vec3(0.0);
    for (int i = 0; i < uNumLensedBodies && i < MAX_LENSED_BODIES; ++i) {
        float t;
        vec3 norm;
        if (intersectSphere(camRel, rayDir, uLensedBodyPos[i], uLensedBodyRadius[i], t, norm)) {
            if (t < fgClosestT) {
                fgClosestT = t;
                fgHitIdx = i;
                fgHitNorm = norm;
            }
        }
    }

    if (fgHitIdx >= 0) {
        // Direct foreground body occlusion in front of black hole
        vec3 col = uLensedBodyColor[fgHitIdx];
        if (uLensedBodyIsStar[fgHitIdx] == 1) {
            float mu = clamp(dot(fgHitNorm, -rayDir), 0.0, 1.0);
            float limb = (1.0 + 2.0 * mu) / 3.0;
            FragColor = vec4(col * uLensedBodyLum[fgHitIdx] * limb * 3.5, 1.0);
        } else {
            float diff = max(0.0, dot(fgHitNorm, -rayDir));
            FragColor = vec4(col * (0.2 + 0.8 * diff), 1.0);
        }
        BrightColor = vec4(max(vec3(0.0), FragColor.rgb - vec3(1.10)) * 0.45, 1.0);
        return;
    }

    // ── Geodesic Light Propagation ───────────────────────────────────────────────
    float bProj = dot(camRel, rayDir);
    vec3 bPerpVec = camRel - bProj * rayDir;
    float bParam = length(bPerpVec);

    vec3 diskRadiance = vec3(0.0);
    float diskAlpha = 0.0;
    bool rayCaptured = false;
    vec3 finalExitDir = rayDir;
    vec3 finalExitPos = camRel;

    // Determine if ray passes through strong gravitational field
    bool entersStrongField = (camDist < rBound) || (bProj < 0.0 && bParam < rBound);

    if (entersStrongField) {
        vec3 rayPos;
        if (camDist >= rBound) {
            float cVal = dot(camRel, camRel) - rBound * rBound;
            float disc = bProj * bProj - cVal;
            float tEnter = -bProj - sqrt(max(0.0, disc));
            rayPos = camRel + rayDir * max(0.0, tEnter);
        } else {
            rayPos = camRel;
        }

        vec3 curPos = rayPos;
        vec3 curDir = rayDir;
        float minRayDist = length(rayPos);
        float accumulatedAngle = 0.0;
        int maxSteps = (uVisualMode >= 2) ? 144 : (uVisualMode == 1 ? 112 : 72);

        for (int step = 0; step < maxSteps; ++step) {
            float r = length(curPos);
            minRayDist = min(minRayDist, r);

            // 1. Relativistic Capture: Inward ray inside photon sphere or horizon cannot escape
            if (r <= rHorizon * 1.01 || (r <= uPhotonRadius * 1.01 && dot(curPos, curDir) < 0.0)) {
                rayCaptured = true;
                break;
            }

            // 2. Escape Condition
            if (r > rBound && dot(curPos, curDir) > 0.0) {
                break;
            }

            // 3. Adaptive Step Size
            float dt;
            if (r < rs * 3.0) {
                dt = clamp(0.038 * r * sqrt(r / rs), 0.005 * rs, 0.12 * rs);
            } else if (r < rs * 8.0) {
                dt = clamp(0.065 * r, 0.04 * rs, 0.30 * rs);
            } else {
                dt = clamp(0.12 * r, 0.18 * rs, 0.08 * rBound);
            }

            // 4. RK4 Geodesic Integrator with Kerr Gravitomagnetic Frame-Dragging
            vec3 k1_v = curDir;
            vec3 k1_a = computeGeodesicAcceleration(curPos, curDir, rs, uSpinParam, spinNorm);

            vec3 p2 = curPos + 0.5 * dt * k1_v;
            vec3 v2 = normalize(curDir + 0.5 * dt * k1_a);
            vec3 k2_v = v2;
            vec3 k2_a = computeGeodesicAcceleration(p2, v2, rs, uSpinParam, spinNorm);

            vec3 p3 = curPos + 0.5 * dt * k2_v;
            vec3 v3 = normalize(curDir + 0.5 * dt * k2_a);
            vec3 k3_v = v3;
            vec3 k3_a = computeGeodesicAcceleration(p3, v3, rs, uSpinParam, spinNorm);

            vec3 p4 = curPos + dt * k3_v;
            vec3 v4 = normalize(curDir + dt * k3_a);
            vec3 k4_v = v4;
            vec3 k4_a = computeGeodesicAcceleration(p4, v4, rs, uSpinParam, spinNorm);

            vec3 nextPos = curPos + (dt / 6.0) * (k1_v + 2.0 * k2_v + 2.0 * k3_v + k4_v);
            vec3 nextDir = normalize(curDir + (dt / 6.0) * (k1_a + 2.0 * k2_a + 2.0 * k3_a + k4_a));

            accumulatedAngle += acos(clamp(dot(curDir, nextDir), -1.0, 1.0));

            // 5. Analytic Equatorial Accretion Disk Crossing
            if (uHasDisk && diskAlpha < 0.98) {
                float z0 = dot(curPos, spinNorm);
                float z1 = dot(nextPos, spinNorm);

                if (z0 * z1 <= 0.0 && z0 != z1) {
                    float tHit = clamp(z0 / (z0 - z1), 0.0, 1.0);
                    vec3 hitPos = mix(curPos, nextPos, tHit);
                    vec3 avgDir = normalize(mix(curDir, nextDir, tHit));

                    sampleDiskAtPoint(hitPos, avgDir, rs, rIn, rOut, spinNorm, diskRadiance, diskAlpha);
                }
            }

            curPos = nextPos;
            curDir = nextDir;
        }

        finalExitPos = curPos;
        finalExitDir = curDir;

        // Razor-thin photon ring highlight at critical boundary
        if (!rayCaptured && minRayDist < rs * 1.62 && minRayDist > rHorizon && accumulatedAngle > 1.9 * PI) {
            float turns = clamp((accumulatedAngle - 1.9 * PI) / (1.2 * PI), 0.0, 1.0);
            float ringPeak = pow(turns, 3.0) * smoothstep(rs * 1.62, rs * 1.50, minRayDist);
            diskRadiance += vec3(1.0, 0.98, 0.94) * ringPeak * (uHasDisk ? 3.0 : 1.5) * (1.0 - diskAlpha);
        }

        // Seamless C1 Hermite transition across strong-to-weak field boundary
        if (!rayCaptured) {
            float deflAngle = (2.0 * rs) / max(bParam, 1e-4);
            vec3 bDir = (bParam > 1e-5) ? normalize(bPerpVec) : vec3(0.0, 1.0, 0.0);
            vec3 weakDir = normalize(rayDir - deflAngle * bDir);
            vec3 weakPos = bPerpVec;

            float w = smoothstep(rBound * 0.70, rBound, bParam);
            finalExitPos = mix(curPos, weakPos, w);
            finalExitDir = normalize(mix(curDir, weakDir, w));
        }
    } else {
        // Outside strong-field zone (Weak-Field Einstein Deflection):
        // Deflection angle alpha = 4GM / (c^2 * b) = 2 rs / b
        // Smoothly curves rays with zero discontinuity, zero discard, and zero circular boundary
        float bClamped = max(bParam, rBound);
        float deflAngle = (2.0 * rs) / bClamped;
        vec3 bDir = (bParam > 1e-5) ? normalize(bPerpVec) : vec3(0.0, 1.0, 0.0);
        finalExitDir = normalize(rayDir - deflAngle * bDir);
        finalExitPos = bPerpVec;
    }

    vec3 finalColor = vec3(0.0);
    vec3 brightColor = vec3(0.0);

    if (rayCaptured) {
        // 100% OPAQUE BLACK HOLE SHADOW:
        // No background star, skybox, or outside light can penetrate the event horizon.
        // Only foreground accretion disk emission is visible.
        finalColor = diskRadiance;
        brightColor = max(vec3(0.0), diskRadiance - vec3(1.10)) * 0.45;
    } else {
        // 1. Sample lensed celestial bodies along deflected ray
        vec3 lensedBodiesRadiance = sampleLensedBodies(finalExitPos, finalExitDir, 1000.0);

        // 2. Sample lensed background skybox
        vec3 skyColor = sampleSkyboxColor(finalExitDir);

        // 3. Composite lensed scene
        vec3 backgroundRadiance = skyColor + lensedBodiesRadiance;
        finalColor = diskRadiance + (1.0 - diskAlpha) * backgroundRadiance;

        // HDR Bright-Pass extraction for bloom
        vec3 excessLum = max(vec3(0.0), finalColor - vec3(1.10));
        brightColor = excessLum * 0.45;
    }

    FragColor = vec4(finalColor, 1.0);
    BrightColor = vec4(brightColor, 1.0);
}
