[中文](./README.md)

# eext-simulation-with-simulide

A WebAssembly circuit simulation tool based on the SimulIDE engine, providing powerful browser-based circuit simulation capabilities for EasyEDA Professional Edition (EasyEDA Professional Edition).

This project is an EasyEDA extension plugin that compiles the SimulIDE C++ simulation engine into WebAssembly, enabling real-time circuit simulation in the browser.

## ✨ Features

- **WebAssembly Compilation** - Compiles C++ simulation engine to WASM using Emscripten for efficient browser execution

- **Real-time Simulation** - Event-driven simulation engine supporting real-time interaction and dynamic component property updates

- **Rich Component Library** - Supports BJT, MOSFET, diodes, resistors, inductors, logic gates, microcontrollers, and more

- **EasyEDA Extension** - Integrated as an extension plugin, communicating with the EasyEDA platform via standard API

- **Data Visualization** - JSON format simulation data output, pushed to EasyEDA interface in real-time

- **High Performance** - Written in C++20, O3 optimized, providing native-level computation performance

- **Cross-platform** - Supports Windows, Linux, macOS builds and deployment

- **Extensible** - Event-driven plugin architecture, easy to extend and maintain

### How the Extension Plugin Works

  LCEDA Professional Edition Platform

    - Extension Manager

      - simulation-with-simulide extension plugin

        - TypeScript extension code (src/index.ts)

        - Event listener registration

        - Simulation control logic

        - Data push processing

      - WebAssembly Module (WASM)

        - C++ Engine

        - Circuit simulation computation

        - Component state management

## 📦 Prerequisites

### Required Tools

| Tool       | Version   | Purpose              | Link                                      |
| ---------- | --------- | -------------------- | ----------------------------------------- |
|            |           |                      |                                           |
| CMake      | 3.15+     | Build configuration  | [cmake.org](https://cmake.org/)           |
| Emscripten | Latest    | WASM compilation     | [emscripten.org](https://emscripten.org/) |
| GCC        | 15.1.0+   | C++ compilation (local build) | [MSYS2](https://www.msys2.org/)           |
| Python     | 3.x       | Run test server      | [python.org](https://www.python.org/)     |

### Environment Setup

**Windows Users (MSYS2 recommended)**:

```bash

# After installing MSYS2, run in MSYS2 terminal

pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake

```

**Installing Emscripten**:

```bash

# Clone emsdk

git clone https://github.com/emscripten-core/emsdk.git

cd emsdk

# Install latest version

./emsdk install latest

./emsdk activate latest

# Configure environment variables (run before each use)

source ./emsdk_env.sh  # Linux/macOS

# Or

emsdk_env.bat          # Windows

```
## 🛠️ Build Steps

### 1. Configure EMSDK Path

Edit the build script to set the Emscripten SDK path:

**Linux/macOS**:

```bash

# Edit build-wasm.sh

export EMSDK="/path/to/your/emsdk"

```

**Windows**:

```batch

# Edit build-wasm.bat

set EMSDK=F:\Program Files (x86)\emsdk\emsdk

```

Or the script will auto-detect common locations. If detection fails, set the environment variable manually.

### 2. Execute Build

**Using build script (recommended)**:

```bash

# Linux/macOS

chmod +x build-wasm.sh

./build-wasm.sh

# Windows (MSYS2/Git Bash)

bash build-wasm.sh

# Windows (CMD)

build-wasm.bat

```

**Clean and rebuild**:

```bash

./build-wasm.sh clean

```

### 3. Build Output

After a successful build, the following files will be generated in the `ouput/` directory:

- `lceda-pro-sim-server.js` - JavaScript interface for WASM module

- `lceda-pro-sim-server.wasm` - WebAssembly binary file

**Note: If you need to package WASM files for plugin creation, transfer these two files to the wasm folder in the root directory for extension generation.**

### Manual Build (Advanced Users)

```bash

# Activate Emscripten environment

source /path/to/emsdk/emsdk_env.sh

For Windows(cmd), use emsdk_env.bat

# Create build directory

mkdir -p build-wasm

cd build-wasm

# Configure CMake

emcmake cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

# Compile (multi-core acceleration)

make -j$(nproc)

```

### TypeScript Extension Build

After completing the WASM build, build the TypeScript extension code:

```bash

# 1. Install dependencies

npm install

# 3. Full build (compile + package extension)

npm run build

```

**Build Output**:

- `dist/index.js` - Compiled extension code

- `simulation-with-simulide_v1.0.0.eext` - Installable extension package

## 📦 Extension Packaging

The extension is packaged as a `.eext` file and can be installed via the Extension Manager in LCEDA's advanced menu options.

## 🛠️ Build Steps

This project consists of two independent build processes:

1. **WASM Build** - Compile C++ simulation engine to WebAssembly

2. **Extension Build** - Compile TypeScript and package as `.eext` extension package

## 🚀 Quick Start

### Using the Extension in EasyEDA

1. **Build the extension package**

   ```bash

   npm install

   npm run build

   ```

   This generates `simulation-with-simulide_v1.0.0.eext` file

2. **Install in EasyEDA**

   - Open EasyEDA Professional Edition

   - Go to Extension Manager

   - Click "Install Extension"

   - Select the generated `.eext` file

   - Restart EasyEDA

3. **Use the extension**

  - Create a new simulation schematic in EDA

  - Ready to use


### Local Development Testing

#### 1. Start Local Test Server

After building, use the Python server for testing:

```bash

# Option 1: Use start-server script (Windows)

start-server.bat

# Option 2: Run directly with Python

python serve.py

# Option 3: Use Python 3 (Linux/macOS)

python3 serve.py

```

The server will start at `http://localhost:8080`.

### 2. Test in Browser

Open browser and visit:

```

http://localhost:8080/test.html


```

### 3. Test Circuit Simulation

On the test page:

1. **Load circuit file**

   - Click the upload area to select a circuit file (supports `.cir`, `.txt`, `.net` formats)

   - Or drag and drop file to upload area

2. **Load circuit**

   - Click "Load Circuit" button to load circuit into simulation engine

3. **Start simulation**

   - Click "Start Simulation" to begin simulation

   - View simulation status, time, and frame rate in real-time

4. **Stop simulation**

   - Click "Stop Simulation" to stop simulation

## 📜 License

This project uses a dual license model:

1. **Original Simulide code**

   - Copyright Santiago González

   - Licensed under GNU Affero General Public License v3.0 (AGPLv3)

   - See [copyright.txt](copyright.txt)

2. **Modified and new code**

   - Copyright EasyEDA & JLC Technology Group

   - Licensed under GNU General Public License v3.0 (GPLv3)

### Usage Requirements

- Any modified version must also be open-sourced under the AGPLv3 + GPLv3 dual license

- If provided as a network service, source code must be made available (per AGPLv3 requirements)

- All original copyright notices and license notices must be retained

For full license content, see the [LICENSE](LICENSE) file.

## 🙏 Acknowledgements

- **SimulIDE Project** ([simulide.com](https://simulide.com/p/)) - Thanks for providing the excellent circuit simulation engine

- **Emscripten Team** - For enabling C++ code to run in the browser

- All project contributors and the open-source community for their support

## 📚 Related Resources

- [SimulIDE Official Website](https://simulide.com/)

- [WebAssembly Official Documentation](https://webassembly.org/)

- [Emscripten Documentation](https://emscripten.org/docs/)
