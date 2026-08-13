# AstroGenesis

**AstroGenesis** is a scientifically grounded, real-time physically accurate Space Simulation Engine built in C++17. The engine is designed to simulate celestial bodies, orbital mechanics, stellar evolution, planetary atmospheres, and complex astrophysical phenomena with high physical realism, modular architecture, and GPU acceleration.

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

## 📜 License & Acknowledgments

* Designed for physical realism and educational space simulation.
* Built using open-source C++ libraries: [GLFW](https://www.glfw.org/), [GLM](https://github.com/g-truc/glm), [GLAD](https://glad.dav1d.de/), and [Dear ImGui](https://github.com/ocornut/imgui).
