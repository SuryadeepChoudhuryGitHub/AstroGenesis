# AstroGenesis

**AstroGenesis** is a scientifically grounded, real-time physically accurate Space Simulation Engine built in C++17. The engine is designed to simulate celestial bodies, orbital mechanics, stellar evolution, planetary atmospheres, and complex astrophysical phenomena with high physical realism, modular architecture, and GPU acceleration.

---

## 📸 Screenshots & Engine Interface

### 1. Real-Time 3D N-Body Simulation & Telemetry Dashboard
> *Interactive 3D viewport simulating multi-body orbital trajectories with live Einstein 1PN Post-Newtonian relativistic corrections, high-precision Keplerian orbital telemetry, time dilation tracking, and dynamic parameter tuning.*

![3D N-Body Orbit Simulation and Telemetry Dashboard](assets/images/Screenshot%200.png)

---

### 2. Astronomical Explorer & NASA/JPL Catalog Discovery
> *Integrated catalog explorer querying the local SQLite database populated with real-world astronomical data from NASA JPL Horizons, JPL Small-Body Database (SBDB), and NASA Exoplanet Archive across solar, asteroid, and multi-star exoplanetary systems.*

![Astronomical Explorer and Catalog Discovery](assets/images/Screenshot%201.png)

---

### 3. Celestial Object Workspace & Physical Property Editor
> *Interactive celestial body inspector and parameter editor allowing real-time creation and tuning of planetary geophysics, orbital parameters, chemical compositions, and immediate testing in the live simulation.*

![Celestial Object Workspace and Physical Property Editor](assets/images/Screenshot%202.png)

---

## 🌌 Project Overview & Objective

The goal of AstroGenesis is to bridge real-time interactivity with astrophysical rigor—drawing inspiration from platforms like *SpaceEngine*, *Universe Sandbox*, and *Kerbal Space Program*, but with an increased focus on deep physical and astrophysical simulation.

### Key Simulation Modules & Features

1. **Celestial Objects**: Complete parameterization (Mass, Radius, Surface Gravity, Albedo, Spectral Type, Luminosity, Habitability Index, etc.) for Stars, Planets, Moons, Black Holes, Pulsars, Quasars, Nebulae, and Galaxies.
2. **Physics Engine**:
   - N-body Newtonian Gravitational Simulation.
   - Lagrange Points, Hill Spheres, Orbital Resonances.
   - General Relativity Approximations (Gravitational Lensing, Time Dilation, Event Horizons).
   - Orbital Mechanics (Elliptical, Hyperbolic, Inclined orbits, Perturbations).
3. **Planetary Physics & Atmosphere**:
   - Tectonic activity, volcanoes, erosion, climate, and seasonal simulation.
   - Multi-gas atmospheric compositions (N₂, O₂, CO₂, CH₄, H₂, He, H₂O).
   - Greenhouse effect, atmospheric escape, pressure gradients, and dynamic cloud/storm formation.
4. **Stellar Evolution**:
   - Full life cycle simulation (Protostar → Main Sequence → Red Giant → Planetary Nebula/Supernova → White Dwarf / Neutron Star / Black Hole).
   - Nuclear fusion, mass loss, luminosity, and spectral shifts.
5. **Thermodynamics & Radiation**:
   - Incoming radiation, Stefan-Boltzmann Law, blackbody radiation, thermal inertia.
   - Full spectrum radiation (Visible, IR, UV, X-Ray, Gamma, Cosmic Rays, Solar Wind).
6. **Relativistic & Light Effects**:
   - Doppler shifts (Redshift/Blueshift), Lorentz contraction, PBR rendering, volumetric lighting, and atmospheric scattering.
7. **Procedural Generation**:
   - Noise-based terrain generation (mountains, canyons, oceans, craters).
   - Galaxy generation (spiral, elliptical, irregular) housing clusters and billions of stars.
8. **Collisions & Interiors**:
   - Internal layer simulation (Core, Mantle, Crust, Atmosphere).
   - Momentum-conserving impact physics, debris field creation, and fragmentation.
9. **Time Controls**:
   - Flexible time scaling (Pause, Real-Time, 10x to 1 Billionx, and Reverse Time).
10. **Spacecraft & AI Navigation**:
    - Thrust, Delta-$v$, orbital insertion, gravity assists, and trajectory prediction AI.

---

## 🏗️ Project Architecture & Tech Stack

The engine is built using standard **C++17** and structured around a modular architecture:

* **Graphics & Windowing**: OpenGL 3.3+ / GLAD, GLFW
* **Mathematics**: GLM (OpenGL Mathematics)
* **User Interface**: Dear ImGui
* **Build System**: CMake (v3.16+)

```text
AstroGenesis/
├── assets/               # Shader files, textures, models, and presets
├── external/             # Submodule dependencies (GLFW, GLM, GLAD, ImGui)
├── src/
│   ├── core/             # Application lifecycle, window management, time system
│   ├── renderer/         # Camera, Shader management, PBR & OpenGL rendering pipeline
│   ├── simulation/       # Celestial bodies, N-body physics, orbital mechanics
│   ├── ui/               # ImGui overlay, control panels, metrics
│   └── main.cpp          # Entry point
├── watch_and_build.ps1   # Auto-build watcher script for live C++ updates
└── CMakeLists.txt        # Build system configuration
```

---

## 🛠️ Prerequisites

Before building, ensure you have the following installed on your system:

* **C++ Compiler** supporting C++17:
  * **Windows**: MinGW-w64 (GCC 10+) or Visual Studio 2022 / MSVC.
  * **Linux**: GCC 9+ or Clang 10+.
* **CMake** (v3.16 or higher).
* **OpenGL 3.3** compatible graphics card and drivers.

---

## ⚙️ Building the Project

### Option A: Command Line (MinGW Makefiles - Recommended)

1. Clone or navigate to the project root directory.
2. Configure the project:
   ```powershell
   cmake -B build -G "MinGW Makefiles"
   ```
3. Build the executable:
   ```powershell
   cmake --build build
   ```
4. The executable will be generated at `./build/AstroGenesis.exe`.

### Option B: Visual Studio (Windows GUI)

1. Generate Visual Studio project files:
   ```powershell
   cmake -B build -G "Visual Studio 17 2022"
   ```
2. Open `./build/AstroGenesis.sln` in Visual Studio.
3. Set **AstroGenesis** as the Startup Project and press **F5** to build and run.

### Option C: Live Auto-Build Watcher (PowerShell)

If you are modifying `.cpp` or `.hpp` files frequently, you can run the included file watcher script:

```powershell
.\watch_and_build.ps1
```
This script monitors the `src/` folder and automatically re-compiles the binary whenever changes are saved.

### Option D: macOS (Apple Silicon / Intel)

1. Install the Xcode Command Line Tools if needed:
   ```bash
   xcode-select --install
   ```
2. Configure the project with CMake:
   ```bash
   cmake -S . -B build
   ```
3. Build the executable:
   ```bash
   cmake --build build -- -j4
   ```
4. Run the program:
   ```bash
   ./build/AstroGenesis
   ```

You can also generate an Xcode project if you prefer working in Xcode:

```bash
cmake -S . -B build-xcode -G Xcode
```

---

## 🚀 Running the Engine

After building, run the executable from the project root:

* **MinGW / Make**:
  ```powershell
  .\build\AstroGenesis.exe
  ```
* **Visual Studio**:
  ```powershell
  .\build\Debug\AstroGenesis.exe
  ```

---

## 🐛 Debugging Guide

### 1. Command-Line Debugging with GDB (MinGW)
If you built using MinGW and want to inspect crashes or memory faults:
```powershell
# Build with debug symbols
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Launch GDB
gdb .\build\AstroGenesis.exe
(gdb) run
```

### 2. VS Code Debugging (`launch.json`)
Create or use `.vscode/launch.json` to debug directly inside Visual Studio Code:
```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug AstroGenesis",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/AstroGenesis.exe",
      "args": [],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}",
      "environment": [],
      "externalConsole": false,
      "MIMode": "gdb",
      "miDebuggerPath": "C:/msys64/ucrt64/bin/gdb.exe"
    }
  ]
}
```

### 3. Visual Studio Native Debugger
Open `build/AstroGenesis.sln` in Visual Studio, set breakpoints in `src/main.cpp` or physics source files, and launch with **Local Windows Debugger (F5)**.

---

---

## 🛰️ External Astronomical Data & Database System

AstroGenesis uses a **data-driven architecture** backed by a normalized local SQLite database (`data/astrogenesis.db`). The physics simulation and UI query the database dynamically, with zero hardcoded celestial object parameters.

### 🏛️ Architecture & Data Flow

```
External Sources (JPL Horizons, JPL SBDB, NASA Exoplanet)
       ↓ HTTPS (Native WinHTTP)
Data Providers (IAstronomicalDataProvider)
       ↓ Parse, Schema Validation, Unit Normalization
Data Manager (Async Worker Threads, Caching, Audit Logging)
       ↓ ACID SQL Transactions
SQLite Database (data/astrogenesis.db)
       ↓ Hydrated In-Memory Entities
Data Access Layer (Object, Ephemeris, Validation Repositories)
       ↓
AstroGenesis Physics Simulation Engine (1PN Einstein GR) & UI
```

---

### 🗄️ Normalized SQLite Database Schema

The database (`data/astrogenesis.db`) is structured into 11 normalized tables with foreign key cascading and multi-column indexes:

| Table | Description | Key Fields |
| :--- | :--- | :--- |
| `objects` | Master entity records | `id`, `slug`, `name`, `type`, `parent_object_id`, `category`, `is_synthetic`, `color`, `texture_path` |
| `physical_properties` | SI physical parameters | `object_id`, `mass_kg`, `radius_m`, `albedo`, `greenhouse_k`, `luminosity_w`, `axial_tilt_deg`, `rotation_period_hours`, `surface_gravity_mps2`, `escape_velocity_mps`, `rings_json`, `source_id`, `source_record_id` |
| `orbital_elements` | Keplerian orbital elements | `object_id`, `epoch_jd`, `semi_major_axis_m`, `semi_major_axis_au`, `eccentricity`, `inclination_deg`, `long_ascending_node_deg`, `arg_periapsis_deg`, `mean_anomaly_deg`, `orbital_period_days`, `reference_frame` |
| `state_vectors` | 3D Cartesian vectors | `object_id`, `epoch_jd`, `pos_x_m`, `pos_y_m`, `pos_z_m`, `vel_x_mps`, `vel_y_mps`, `vel_z_mps`, `reference_frame` |
| `ephemeris_records` | Ground-truth time-series | `object_id`, `target_name`, `epoch_utc`, `epoch_jd`, `pos_x_m`, `pos_y_m`, `pos_z_m`, `vel_x_mps`, `vel_y_mps`, `vel_z_mps`, `reference_frame` |
| `composition` | Atmospheric / elemental | `object_id`, `element_or_compound`, `percentage`, `color_r`, `color_g`, `color_b`, `color_a` |
| `data_sources` | Registered data providers | `id`, `name`, `base_url`, `description`, `is_official` |
| `data_imports` | Import audit logs | `id`, `source_id`, `target_object`, `status`, `records_count`, `details`, `timestamp` |
| `simulation_runs` | Simulation run tracking | `id`, `name`, `integrator_type`, `start_epoch_jd`, `time_scale`, `gr_enabled`, `total_sim_seconds` |
| `simulation_states` | Periodic state snapshots | `id`, `run_id`, `object_id`, `sim_time_seconds`, `pos_x_m`, `vel_x_mps`, `energy_joules`, `angular_momentum` |
| `validation_results` | Benchmark comparisons | `id`, `run_id`, `object_id`, `epoch_jd`, `sim_pos`, `real_pos`, `pos_error_m`, `pos_relative_error`, `vel_error_mps`, `energy_drift_pct`, `gr_mode` |

---

### 🌐 Supported External Data Providers & API Endpoints

1. **NASA JPL Horizons API**:
   - **Endpoint**: `https://ssd.jpl.nasa.gov/api/horizons.api`
   - **Data**: High-precision state vectors ($X, Y, Z, V_X, V_Y, V_Z$ in ICRF/Barycentric), Keplerian orbital elements, physical constants (mass, radius, density, rotation period).
   - **Usage**: Planetary ephemerides, moons, asteroids, comets, and time-series benchmark truth vectors.

2. **NASA JPL Small-Body Database (SBDB)**:
   - **Endpoint**: `https://ssd-api.jpl.nasa.gov/sbdb.api`
   - **Data**: >1.3 million asteroids and comets, orbital parameters ($a, e, i, \Omega, \omega, M, P$), physical parameters (diameter, geometric albedo, rotation period, $GM$, spectral taxonomy).
   - **Usage**: Main-belt asteroids, near-Earth objects (NEAs), Trojans, Centaurs, comets.

3. **NASA Exoplanet Archive (TAP)**:
   - **Endpoint**: `https://exoplanetarchive.ipac.caltech.edu/TAP/sync`
   - **Data**: Multi-planet exoplanetary systems, host star parameters (mass, radius, $T_{\text{eff}}$, luminosity, spectral type), planet parameters (mass $M_\oplus$, radius $R_\oplus$, orbital period, semi-major axis, eccentricity, equilibrium temperature).
   - **Usage**: TRAPPIST-1, Proxima Centauri, Kepler-186, and confirmed exoplanets.

---

### 📐 Centralized Unit System

All external data is validated and converted into standardized internal SI units upon import:

| Quantity | External Units | Internal SI Unit | Conversion Factor |
| :--- | :--- | :--- | :--- |
| **Distance** | $\text{AU}$, $\text{km}$, $\text{light-years}$, $\text{parsecs}$ | Meters ($\text{m}$) | $1\text{ AU} = 149,597,870,700\text{ m}$ |
| **Velocity** | $\text{km/s}$, $\text{AU/day}$ | Meters per second ($\text{m/s}$) | $1\text{ km/s} = 1,000\text{ m/s}$ |
| **Mass** | $M_\oplus$ (Earth), $M_{\text{Jup}}$ (Jupiter), $M_\odot$ (Solar) | Kilograms ($\text{kg}$) | $1 M_\oplus = 5.97219 \times 10^{24}\text{ kg}$ |
| **Radius** | $R_\oplus$ (Earth), $R_{\text{Jup}}$ (Jupiter), $R_\odot$ (Solar), $\text{km}$ | Meters ($\text{m}$) | $1 R_\oplus = 6,371,000\text{ m}$ |
| **Angle** | Degrees ($^\circ$), Arcseconds ($''$) | Radians ($\text{rad}$) | $1^\circ = \pi / 180\text{ rad}$ |
| **Time** | Julian Date ($\text{JD}$), Calendar UTC | Seconds ($\text{s}$) & $\text{JD}$ | $1\text{ Julian Day} = 86,400\text{ s}$ |

---

### 🔬 Real vs. Simulation Validation Methodology

The `ValidationEngine` compares simulated trajectories $\vec{R}_{\text{sim}}(t)$ directly against NASA JPL Horizons ground truth observations $\vec{R}_{\text{real}}(t)$:

1. **Absolute Position Error**:
   $$\Delta R(t) = \|\vec{R}_{\text{sim}}(t) - \vec{R}_{\text{real}}(t)\|$$

2. **Relative Error**:
   $$\text{Relative Error}(t) = \frac{\|\vec{R}_{\text{sim}}(t) - \vec{R}_{\text{real}}(t)\|}{\|\vec{R}_{\text{real}}(t)\|}$$

3. **Velocity Error**:
   $$\Delta V(t) = \|\vec{V}_{\text{sim}}(t) - \vec{V}_{\text{real}}(t)\|$$

4. **Energy Conservation Drift**:
   $$\text{Drift}_E(t) = \frac{|E(t) - E_0|}{|E_0|} \times 100\%$$

5. **General Relativity Validation (Mercury Perihelion Test)**:
   - Evaluates the Einstein 1PN Post-Newtonian acceleration:
     $$\vec{a}_{\text{1PN}} = \frac{G M}{c^2 r^3} \left[ \left( 4 \frac{G M}{r} - v^2 \right) \vec{r} + 4 (\vec{r} \cdot \vec{v}) \vec{v} \right]$$
   - Computes Mercury's perihelion advance ($+42.98''/\text{century}$) and confirms match with JPL Horizons ephemerides vs. Newtonian $0.00''/\text{century}$.

---

## 📜 License & Acknowledgments

* Designed for physical realism and educational space simulation.
* Built using open-source C++ libraries: [GLFW](https://www.glfw.org/), [GLM](https://github.com/g-truc/glm), [GLAD](https://glad.dav1d.de/), [Dear ImGui](https://github.com/ocornut/imgui), [SQLite](https://www.sqlite.org/), and [nlohmann/json](https://github.com/nlohmann/json).
* Astronomical data provided courtesy of NASA Jet Propulsion Laboratory (JPL) Horizons, Small-Body Database (SBDB), and the NASA Exoplanet Archive.

