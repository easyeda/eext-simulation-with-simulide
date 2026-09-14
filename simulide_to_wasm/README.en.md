# simulide_to_wasm

WebAssembly build project for the SimulIDE simulation engine. Compiles the C++-based simulation core into a browser-runnable WASM module that powers the upper-layer EasyEDA extension.

## 📖 Overview

This directory is the **C++/WASM sub-project** of `eext-simulation-with-simulide`. It is responsible for:

- Compiling the SimulIDE simulation engine (C++20) to WebAssembly
- Exposing JS-callable simulation control APIs (load circuit, start/stop/pause, read/write simulation data, etc.)
- Embedding simulation configuration data into the WASM virtual filesystem
- Communicating with JavaScript through Emscripten's `ccall` / `cwrap` interfaces

The build artifacts (`lceda-pro-sim-server.js` and `lceda-pro-sim-server.wasm`) are loaded and executed by the upper-layer extension plugin.

## 📂 Directory Structure

```
simulide_to_wasm/
├── CMakeLists.txt          # CMake build config (WASM / Native dual mode)
├── build-wasm.sh           # Linux/macOS/MSYS2 build script
├── build-wasm.bat          # Windows CMD build script
├── start-server.sh         # Start local test server (Linux/macOS)
├── start-server.bat        # Start local test server (Windows)
├── serve.py                # Python local static server
├── test.html               # Browser debug page (load circuit / start sim / visualize data)
├── config/
│   └── data/               # Simulation engine config (embedded into WASM via --embed-file)
├── src/
│   ├── server.cpp          # WASM entry, exports C interfaces for JS
│   ├── cirSim/             # Ported SimulIDE circuit simulation core
│   └── public/include/     # Public headers
└── output/                 # Build artifacts (lceda-pro-sim-server.js / .wasm)
```

## 🧩 Exposed APIs

The following C interfaces are exported via `EMSCRIPTEN_KEEPALIVE` and can be called from JS through `Module.cwrap` / `Module.ccall`:

| Function                 | Description                                          |
| ------------------------ | ---------------------------------------------------- |
| `loadCircuitFromFile`    | Load a netlist file and create a simulation instance |
| `startSimulation`        | Start the simulation                                 |
| `stopSimulation`         | Stop the simulation                                  |
| `pauseSimulation`        | Pause the simulation                                 |
| `resumeSimulation`       | Resume the simulation                                |
| `stepSimulation`         | Advance the simulation by one step                   |
| `getSimulationState`     | Get the current simulation state                     |
| `setSimulationSpeed`     | Set the simulation speed multiplier                  |
| `getSimulationTime`      | Get the current simulation time                      |
| `getSimulationData`      | Return incremental simulation data as a JSON string (meters / wires / statuses) |
| `updateCircuitData`      | Dynamically update component attributes (e.g. potentiometer resistance, switch state) |
| `getVersion`             | Get the engine version string                        |

Sample simulation data JSON:

```json
{
  "Meter_1": [[0.001, 3.3], [0.002, 3.28]],
  "Wire_0": [[0.001, 5.0]],
  "Led_0": [[0.001, "on"]]
}
```

## 📦 Prerequisites

| Tool         | Version    | Purpose                                |
| ------------ | ---------- | -------------------------------------- |
| CMake        | 3.15+      | Build configuration                    |
| Emscripten   | latest     | Compile C++ to WASM                    |
| Python       | 3.x        | Run CMake (`emcmake`) and the local test server |

> Native (non-EMSCRIPTEN) builds are only for debugging — production use of the extension relies on the WASM artifacts.

### Install Emscripten

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest

# Activate before each use
source ./emsdk_env.sh   # Linux/macOS
emsdk_env.bat           # Windows
```

## 🛠️ Build Steps

### 1. Configure EMSDK Path

`build-wasm.sh` automatically searches common emsdk install locations in this order:

- Environment variable `EMSDK`
- Linux/macOS: `$HOME/emsdk`, `/opt/emsdk`
- Windows: `/f/Program Files (x86)/emsdk/emsdk`, `/c/emsdk`, `$HOME/emsdk`

If auto-detection fails, set the path manually:

```bash
# Linux/macOS
export EMSDK="/path/to/emsdk"
./build-wasm.sh

# Windows (CMD)
set EMSDK=F:\Program Files (x86)\emsdk\emsdk
build-wasm.bat
```

### 2. Run the Build

```bash
# Linux/macOS
chmod +x build-wasm.sh
./build-wasm.sh

# Windows (MSYS2 / Git Bash)
bash build-wasm.sh

# Windows (CMD)
build-wasm.bat

# Clean and rebuild
./build-wasm.sh clean
```

After a successful build, artifacts are placed in `output/`:

- `lceda-pro-sim-server.js` — JavaScript interface (Emscripten glue code)
- `lceda-pro-sim-server.wasm` — WebAssembly binary

### 3. Manual Build (Advanced)

```bash
source /path/to/emsdk/emsdk_env.sh   # On Windows, use emsdk_env.bat

mkdir -p build-wasm && cd build-wasm
emcmake cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Key CMake / Emscripten Flags

`CMakeLists.txt` configures the following Emscripten link flags:

- `MODULARIZE=1`, `EXPORT_ES6=1` — Export as an ES Module
- `ENVIRONMENT=web` — Web-only runtime environment
- `ALLOW_MEMORY_GROWTH=1` — Allow dynamic WASM memory growth
- `EXPORTED_RUNTIME_METHODS=["ccall","cwrap","FS"]` — Expose ccall/cwrap/filesystem APIs
- `FORCE_FILESYSTEM=1` + `--embed-file config/data@/config/data` — Embed configuration data into the WASM filesystem

## 🧪 Local Debugging

After the build completes, use the Python static server to load `test.html` for browser-side debugging:

```bash
# Start the server (default port :8080)
python3 serve.py            # or ./start-server.sh / start-server.bat

# Open in browser
http://localhost:8080/test.html
```

`test.html` provides:

- Upload/drag-and-drop netlist files (`.cir` / `.txt` / `.net`)
- Load circuit, start/stop/pause simulation
- Real-time display of simulation time and frame rate
- Live waveform display for each meter/probe

## 🔗 Integration with the Upper-Layer Extension

Build artifacts from this project must be copied into the `wasm/` directory at the repo root, then consumed by the TypeScript extension via `Module.cwrap`:

```js
const Module = await import('./lceda-pro-sim-server.js');
const loadCircuit = Module.cwrap('loadCircuitFromFile', 'number', ['string', 'string']);
const startSim = Module.cwrap('startSimulation', null, []);
const getData = Module.cwrap('getSimulationData', 'string', []);
```

See the root [README](../README.md) for the full extension packaging and release workflow.

## 📜 License

- Original SimulIDE code: AGPLv3 (Copyright © Santiago González)
- New / modified code in this project: GPLv3 (Copyright © EasyEDA & JLC Technology Group)

See [copyright.txt](../copyright.txt) and [LICENSE](../LICENSE) for details.
