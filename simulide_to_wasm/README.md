# eext-simulation-with-simulide
基于 Simulide 引擎的 WebAssembly 电路仿真工具，支持在浏览器中进行实时电路仿真。

## ✨ 特性

- 🚀 **WebAssembly 编译** - 基于 Emscripten 编译为 WASM，可在浏览器中运行
- ⚡ **实时仿真** - 事件驱动的仿真引擎，支持实时交互和动态更新
- 🔌 **丰富的元件库** - 支持 BJT、MOSFET、二极管、电阻、电感等多种电路元件
- 📊 **数据可视化** - JSON 格式的仿真数据输出，便于集成和分析
- 🎯 **高性能** - C++20 编写，优化的计算性能
- 🌐 **跨平台** - 支持 Windows、Linux、macOS 构建

## 📜 开源协议

本项目采用双许可证模式：

1. **原始 Simulide 代码**  
   - 版权归 Santiago González 所有
   - 采用 GNU Affero General Public License v3.0 (AGPLv3) 协议
   - 参见 [copyright.txt](copyright.txt)

2. **修改和新增代码**  
   - 版权归 EasyEDA & JLC Technology Group 所有
   - 采用 GNU General Public License v3.0 (GPLv3) 协议

### 使用要求
- 任何修改后的版本必须同样采用 AGPLv3 + GPLv3 双协议开源
- 如果作为网络服务提供，必须提供源代码（根据 AGPLv3 要求）
- 必须保留所有原始版权声明和许可声明

完整协议内容请查看 [LICENSE](LICENSE) 文件。

## ⚙️ 技术架构

### 核心引擎
- **Simulide 仿真引擎** - 基于 Simulide 的事件驱动仿真核心，支持实时交互
- **WebAssembly 运行时** - 使用 Emscripten 编译，运行在浏览器环境
- **双缓冲机制** - 线程安全的数据交换，支持仿真与数据读取并行

## 📦 前置要求

### 必需工具

| 工具 | 版本要求 | 用途 | 获取链接 |
|------|---------|------|---------|
| CMake | 3.15+ | 构建配置 | [cmake.org](https://cmake.org/) |
| Emscripten | 最新版 | WASM 编译 | [emscripten.org](https://emscripten.org/) |
| GCC | 15.1.0+ | C++ 编译（本地构建） | [MSYS2](https://www.msys2.org/) |
| Python | 3.x | 运行测试服务器 | [python.org](https://www.python.org/) |

### 环境配置

**Windows 用户 (推荐使用 MSYS2)**:
```bash
# 安装 MSYS2 后，在 MSYS2 终端中执行
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake
```

**安装 Emscripten**:
```bash
# 克隆 emsdk
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# 安装最新版本
./emsdk install latest
./emsdk activate latest

# 配置环境变量（每次使用前需要执行）
source ./emsdk_env.sh  # Linux/macOS
# 或
emsdk_env.bat          # Windows
```

## 🛠️ 构建步骤

### 1. 配置 EMSDK 路径

编辑构建脚本，设置 Emscripten SDK 路径：

**Linux/macOS**:
```bash
# 编辑 build-wasm.sh
export EMSDK="/path/to/your/emsdk"
```

**Windows**:
```batch
# 编辑 build-wasm.bat
set EMSDK=F:\Program Files (x86)\emsdk\emsdk
```

或者在脚本中会自动检测常见位置，如果检测失败，请手动设置环境变量。

### 2. 执行构建

**使用构建脚本（推荐）**:

```bash
# Linux/macOS
chmod +x build-wasm.sh
./build-wasm.sh

# Windows (MSYS2/Git Bash)
bash build-wasm.sh

# Windows (CMD)
build-wasm.bat
```

**清理并重新构建**:
```bash
./build-wasm.sh clean
```

### 3. 构建输出

构建成功后，在 `build-wasm/` 目录下会生成以下文件：
- `lceda-pro-sim-server.js` - WASM 模块的 JavaScript 接口
- `lceda-pro-sim-server.wasm` - WebAssembly 二进制文件

### 手动构建（高级用户）

```bash
# 激活 Emscripten 环境
source /path/to/emsdk/emsdk_env.sh
Windows(cmd) 使用emsdk_env.bat

# 创建构建目录
mkdir -p build-wasm
cd build-wasm

# 配置 CMake
emcmake cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

# 编译（使用多核加速）
make -j$(nproc)
```

## 📂 项目结构

```text
sim_eda/
├── src/                      # 源代码目录
│   ├── cirSim/              # 电路仿真模块
│   │   ├── include/         # 仿真引擎头文件
│   │   └── src/             # 仿真引擎源代码
│   ├── public/              # 公共头文件和接口
│   └── server.cpp           # WebAssembly 主入口
├── external/                # 第三方依赖库
├── config/                  # 运行时配置文件
├── build-wasm/             # WASM 构建输出目录（自动生成）
├── build-wasm.sh           # Linux/macOS 构建脚本
├── build-wasm.bat          # Windows 构建脚本
├── serve.py                # 本地测试服务器
├── test.html               # 浏览器测试页面
├── CMakeLists.txt          # CMake 主配置文件
├── LICENSE                 # 开源许可证
├── copyright.txt           # 版权声明
└── README.md               # 项目文档
```

## 🚀 快速开始

### 1. 启动本地测试服务器

构建完成后，使用 Python 服务器进行测试：

```bash
# 方式一：使用 start-server 脚本（Windows）
start-server.bat

# 方式二：使用 Python 直接运行
python serve.py

# 方式三：使用 Python 3（Linux/macOS）
python3 serve.py
```

服务器将在 `http://localhost:8080` 启动。

### 2. 在浏览器中测试

打开浏览器访问：
```
http://localhost:8080/test.html
```

### 3. 测试电路仿真

在测试页面中：

1. **加载电路文件**
   - 点击上传区域选择电路文件（支持 `.cir`, `.txt`, `.net` 等格式）
   - 或拖拽文件到上传区域

2. **加载电路**
   - 点击 "Load Circuit" 按钮加载电路到仿真引擎

3. **启动仿真**
   - 点击 "Start Simulation" 开始仿真
   - 实时查看仿真状态、时间和帧率

4. **停止仿真**
   - 点击 "Stop Simulation" 停止仿真

### 4. API 使用示例

在您的 Web 应用中使用 WASM 模块：

```javascript
// 1. 导入 WASM 模块
import wasmModule from './build-wasm/lceda-pro-sim-server.js';

// 2. 初始化模块
const Module = await wasmModule();

// 3. 加载电路
const result = Module.ccall(
    'loadCircuitFromFile',
    'number',
    ['string', 'string'],
    [fileName, fileContent]
);

// 4. 启动仿真
Module.ccall('startSimulation', null, [], []);

// 5. 获取仿真状态
const state = Module.ccall('getSimulationState', 'number', [], []);

// 6. 获取仿真数据（JSON 格式）
const dataJson = Module.ccall('getSimulationData', 'string', [], []);
const data = JSON.parse(dataJson);

// 7. 单步执行仿真
Module._stepSimulation();

// 8. 获取仿真时间
const time = Module._getSimulationTime();  // 单位：皮秒 (ps)

// 9. 停止仿真
Module.ccall('stopSimulation', null, [], []);
```


## 📝 常见问题

### Q: 构建时找不到 EMSDK？
**A**: 请确保：
1. 已正确安装并激活 Emscripten SDK
2. 设置了 `EMSDK` 环境变量或修改了构建脚本中的路径
3. 使用 `source emsdk_env.sh` 激活环境

### Q: 浏览器无法加载 WASM 模块？
**A**: 请检查：
1. 是否使用了本地服务器（不能直接用 `file://` 协议）
2. 服务器是否正确设置了 CORS 和 MIME 类型
3. 浏览器控制台是否有错误信息

### Q: 仿真运行但没有数据输出？
**A**: 确保：
1. 电路文件格式正确
2. 已正确调用 `loadCircuitFromFile` 加载电路
3. 使用 `getSimulationData` 在正确的时机获取数据
4. 检查浏览器控制台的日志信息

### Q: 如何调试 WASM 模块？
**A**: 可以：
1. 在 CMakeLists.txt 中启用 `-sASSERTIONS=1` 编译选项
2. 使用浏览器开发者工具查看控制台输出
3. 在 C++ 代码中使用 `emscripten_log` 输出调试信息
4. 使用 Chrome 的 WebAssembly 调试工具

## 🙏 致谢

- Simulide 项目 ([simulide.com](https://simulide.com/p/)) - 感谢提供优秀的电路仿真引擎
- 所有项目贡献者和开源社区的支持



