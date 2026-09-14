# simulide_to_wasm

SimulIDE 仿真引擎的 WebAssembly 编译工程，将基于 C++ 的仿真核心编译为可在浏览器中运行的 WASM 模块，为上层 EasyEDA 扩展插件提供电路仿真能力。

## 📖 项目说明

本目录是 `eext-simulation-with-simulide` 项目的 **C++/WASM 子工程**，负责：

- 编译 SimulIDE 仿真引擎（C++20）至 WebAssembly
- 暴露 JS 可调用的仿真控制 API（加载电路、启动/停止/暂停、读写仿真数据等）
- 在 WASM 虚拟文件系统中嵌入仿真所需的配置数据
- 通过 Emscripten 的 `ccall/cwrap` 接口与 JavaScript 通信

编译产物（`lceda-pro-sim-server.js` 与 `lceda-pro-sim-server.wasm`）由上层扩展插件加载运行。

## 📂 目录结构

```
simulide_to_wasm/
├── CMakeLists.txt          # CMake 构建配置（WASM / Native 双模式）
├── build-wasm.sh           # Linux/macOS/MSYS2 构建脚本
├── build-wasm.bat          # Windows CMD 构建脚本
├── start-server.sh         # 启动本地测试服务器（Linux/macOS）
├── start-server.bat        # 启动本地测试服务器（Windows）
├── serve.py                # Python 本地静态服务器
├── test.html               # 浏览器端调试页面（加载电路 / 启动仿真 / 数据可视化）
├── config/
│   └── data/               # 仿真引擎配置数据（通过 --embed-file 嵌入到 WASM）
├── src/
│   ├── server.cpp          # WASM 入口，导出 JS 调用的 C 接口
│   ├── cirSim/             # 移植自 SimulIDE 的电路仿真核心
│   └── public/include/     # 公共头文件
└── output/                 # 构建产物（lceda-pro-sim-server.js / .wasm）
```

## 🧩 对外 API

通过 `EMSCRIPTEN_KEEPALIVE` 导出以下 C 接口，JS 端可通过 `Module.cwrap` / `Module.ccall` 调用：

| 函数                          | 作用                                  |
| --------------------------- | ----------------------------------- |
| `loadCircuitFromFile`       | 加载网表文件并创建仿真实例                       |
| `startSimulation`           | 启动仿真                                |
| `stopSimulation`            | 停止仿真                                |
| `pauseSimulation`           | 暂停仿真                                |
| `resumeSimulation`          | 恢复仿真                                |
| `stepSimulation`            | 单步推进仿真                              |
| `getSimulationState`        | 获取仿真运行状态                            |
| `setSimulationSpeed`        | 设置仿真速度倍率                            |
| `getSimulationTime`         | 获取当前仿真时间                            |
| `getSimulationData`         | 以 JSON 字符串返回增量仿真数据（表/探针/状态）        |
| `updateCircuitData`         | 动态更新元件属性（如电位器阻值、开关状态等）             |
| `getVersion`                | 获取引擎版本字符串                           |

仿真数据 JSON 示例：

```json
{
  "Meter_1": [[0.001, 3.3], [0.002, 3.28]],
  "Wire_0": [[0.001, 5.0]],
  "Led_0": [[0.001, "on"]]
}
```

## 📦 前置要求

| 工具         | 版本要求    | 用途                          |
| ---------- | ------- | --------------------------- |
| CMake      | 3.15+   | 构建配置                        |
| Emscripten | 最新版     | 将 C++ 编译为 WASM              |
| Python     | 3.x     | 运行 CMake（emcmake）与本地测试服务器 |

> 本地原生构建（不带 EMSCRIPTEN）仅供调试使用，扩展插件正式运行依赖 WASM 产物。

### 安装 Emscripten

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest

# 每次使用前激活
source ./emsdk_env.sh   # Linux/macOS
emsdk_env.bat           # Windows
```

## 🛠️ 构建步骤

### 1. 配置 EMSDK 路径

`build-wasm.sh` 会按以下顺序自动搜索 emsdk 安装位置：

- 环境变量 `EMSDK`
- Linux/macOS：`$HOME/emsdk`、`/opt/emsdk`
- Windows：`/f/Program Files (x86)/emsdk/emsdk`、`/c/emsdk`、`$HOME/emsdk`

如自动检测失败，可手动指定：

```bash
# Linux/macOS
export EMSDK="/path/to/emsdk"
./build-wasm.sh

# Windows (CMD)
set EMSDK=F:\Program Files (x86)\emsdk\emsdk
build-wasm.bat
```

### 2. 执行构建

```bash
# Linux/macOS
chmod +x build-wasm.sh
./build-wasm.sh

# Windows (MSYS2 / Git Bash)
bash build-wasm.sh

# Windows (CMD)
build-wasm.bat

# 清理后重新构建
./build-wasm.sh clean
```

构建完成后，产物位于 `output/` 目录：

- `lceda-pro-sim-server.js` — JavaScript 接口（含 Emscripten 胶水代码）
- `lceda-pro-sim-server.wasm` — WebAssembly 二进制

### 3. 手动构建（高级）

```bash
source /path/to/emsdk/emsdk_env.sh   # Windows 使用 emsdk_env.bat

mkdir -p build-wasm && cd build-wasm
emcmake cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### CMake 关键参数

`CMakeLists.txt` 中设置了如下 Emscripten 链接标志：

- `MODULARIZE=1`、`EXPORT_ES6=1` — 以 ES Module 形式导出
- `ENVIRONMENT=web` — 仅支持 Web 运行环境
- `ALLOW_MEMORY_GROWTH=1` — 允许 WASM 内存动态扩容
- `EXPORTED_RUNTIME_METHODS=["ccall","cwrap","FS"]` — 暴露 ccall/cwrap/文件系统 API
- `FORCE_FILESYSTEM=1` + `--embed-file config/data@/config/data` — 将配置数据嵌入 WASM 文件系统

## 🧪 本地调试

构建完成后，可使用 Python 静态服务器加载 `test.html` 进行浏览器端调试：

```bash
# 启动服务（默认监听 :8080）
python3 serve.py            # 或 ./start-server.sh / start-server.bat

# 浏览器访问
http://localhost:8080/test.html
```

`test.html` 提供的调试能力：

- 上传/拖拽网表文件（`.cir` / `.txt` / `.net`）
- 加载电路、启动/停止/暂停仿真
- 实时显示仿真时间、帧率
- 显示各探针/表的实时数据曲线

## 🔗 与上层扩展的集成

本工程的构建产物需拷贝到仓库根目录的 `wasm/` 目录，再由 TypeScript 扩展通过 `Module.cwrap` 等接口调用：

```js
const Module = await import('./lceda-pro-sim-server.js');
const loadCircuit = Module.cwrap('loadCircuitFromFile', 'number', ['string', 'string']);
const startSim = Module.cwrap('startSimulation', null, []);
const getData = Module.cwrap('getSimulationData', 'string', []);
```

详细的扩展打包与发布流程参见根目录 [README](../README.md)。

## 📜 协议

- 原始 SimulIDE 代码：AGPLv3（版权归 Santiago González）
- 本工程新增/修改代码：GPLv3（版权归 EasyEDA & JLC Technology Group）

详见 [copyright.txt](../copyright.txt) 与 [LICENSE](../LICENSE)。
