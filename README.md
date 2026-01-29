  

# eext-simulation-with-simulide

基于 SimulIDE 引擎的 WebAssembly 电路仿真工具，为 EasyEDA 专业版（嘉立创EDA专业版）提供强大的浏览器端电路仿真能力。

本项目是一个 EasyEDA 扩展插件，将 SimulIDE 的 C++ 仿真引擎编译为 WebAssembly，实现了在浏览器中进行实时电路仿真的功能。

## ✨ 特性

- 🚀 **WebAssembly 编译** - 基于 Emscripten 将 C++ 仿真引擎编译为 WASM，在浏览器中高效运行

  

- ⚡ **实时仿真** - 事件驱动的仿真引擎，支持实时交互和动态元件属性更新

  

- 🔌 **丰富的元件库** - 支持 BJT、MOSFET、二极管、电阻、电感、逻辑门、微控制器等多种电路元件

  

- 📦 **EasyEDA 扩展** - 以扩展插件形式集成，通过标准 API 与 EasyEDA 平台通信

  

- 📊 **数据可视化** - JSON 格式的仿真数据输出，实时推送到 EasyEDA 界面

  

- 🎯 **高性能** - C++20 编写，O3 优化，提供原生级别的计算性能

  

- 🌐 **跨平台** - 支持 Windows、Linux、macOS 构建和部署

  

- 🔧 **可扩展** - 基于事件驱动的插件架构，易于扩展和维护

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

## 🔌 扩展系统架构

本项目实现了一个现代化的 EasyEDA 浏览器扩展系统，采用事件驱动架构与 EasyEDA 平台集成。

### 扩展插件工作原理

  LCEDA 专业版平台

   - 扩展管理器

     - simulation-with-simulide 扩展插件

       - TypeScript 扩展代码（src/index.ts）

       - 事件监听器注册

       - 仿真控制逻辑

       - 数据推送处理

    - webAssembly 模块（WASM）

      - c++引擎

      - 电路模拟计算

      - 元件状态管理

  ### 扩展生命周期

1. **安装阶段** - 用户安装 `.eext` 扩展包到 EasyEDA

2. **加载阶段** - EasyEDA 读取 `extension.json` 清单文件

3. **激活阶段** - 触发 `onStartupFinished` 事件，调用扩展的 `activate()` 函数

4. **运行阶段** - 扩展监听仿真事件，调用 WASM 引擎，推送仿真数据

5. **卸载阶段** - 用户卸载扩展，清理所有事件监听器和资源  

## 📦 前置要求

### 必需工具

| 工具         | 版本要求    | 用途           | 获取链接                                      |
| ---------- | ------- | ------------ | ----------------------------------------- |
|            |         |              |                                           |
| CMake      | 3.15+   | 构建配置         | [cmake.org](https://cmake.org/)           |
| Emscripten | 最新版     | WASM 编译      | [emscripten.org](https://emscripten.org/) |
| GCC        | 15.1.0+ | C++ 编译（本地构建） | [MSYS2](https://www.msys2.org/)           |
| Python     | 3.x     | 运行测试服务器      | [python.org](https://www.python.org/)     |

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

source ./emsdk_env.sh  # Linux/macOS

# 或

emsdk_env.bat          # Windows

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

构建成功后，在 `ouput/` 目录下会生成以下文件：

- `lceda-pro-sim-server.js` - WASM 模块的 JavaScript 接口

- `lceda-pro-sim-server.wasm` - WebAssembly 二进制文件

**注：需要打包WASM文件制作插件的话，需要将这两个文件转移到根目录的wasm文件夹中进行插件生成。**

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

### TypeScript 扩展构建 

在完成 WASM 构建后，构建 TypeScript 扩展代码： 

```bash

# 1. 安装依赖

npm install

# 3. 完整构建（编译 + 打包扩展）

npm run build

```

**构建输出**:

- `dist/index.js` - 编译后的扩展代码

- `simulation-with-simulide_v1.0.0.eext` - 可安装的扩展包


## 📦 扩展打包

扩展打包为 `.eext` 文件，可通过 LCEDA菜单选项中的高级选项 => 扩展管理器安装。

## 📂 项目结构

```text

sim_eda/

├── src/                          # TypeScript 扩展源代码

│   ├── index.ts                  # 扩展主入口，事件监听器注册

│   └── assets.d.ts               # 资源文件类型定义

├── simulide_to_wasm/            # C++ 仿真引擎

│   ├── src/

│   │   ├── server.cpp           # WASM 模块入口点

│   │   ├── cirSim/              # 电路仿真模块

│   │   │   ├── include/         # 头文件

│   │   │   └── src/             # 源文件

│   │   └── public/              # 公共接口

│   ├── config/                  # 运行时配置数据（嵌入WASM）

│   ├── external/                # 第三方依赖

│   ├── output/                  # 编译输出目录

│   ├── CMakeLists.txt          # CMake 构建配置

│   ├── build-wasm.sh/.bat      # WASM 构建脚本

│   ├── serve.py                # 本地测试服务器

│   └── test.html               # 浏览器测试页面

├── wasm/                        # 编译好的 WASM 文件

│   ├── lceda-pro-sim-server.js # JavaScript 胶水代码 (108KB)

│   └── lceda-pro-sim-server.wasm # WASM 二进制 (21MB)

├── build/                       # 构建工具

│   └── packaged.ts             # 扩展打包脚本

├── config/                      # 构建配置

│   ├── esbuild.common.ts       # ESBuild 配置（含自定义插件）

│   └── esbuild.prod.ts         # 生产构建脚本

├── locales/                     # 国际化资源

│   ├── en.json                 # 英文翻译

│   ├── zh-Hans.json            # 简体中文翻译

│   └── extensionJson/          # 扩展清单多语言

├── iframe/                      # 嵌入式 iframe 内容

├── images/                      # 扩展资源（图标等）

├── dist/                        # TypeScript 编译输出（自动生成）

├── extension.json              # 扩展清单文件

├── package.json                # NPM 配置

├── tsconfig.json               # TypeScript 配置

├── .edaignore                  # 打包忽略文件列表

├── README.md                   # 项目文档

├── CHANGELOG.md                # 版本历史

├── LICENSE                     # 开源许可证

└── copyright.txt               # 版权声明

```

## 🛠️ 构建步骤

本项目分为两个独立的构建流程：

1. **WASM 构建** - 将 C++ 仿真引擎编译为 WebAssembly

2. **扩展构建** - 编译 TypeScript 并打包为 `.eext` 扩展包

## 🚀 快速开始

### 在 EasyEDA 中使用扩展

1. **构建扩展包**

   ```bash

   npm run build 

   ```

   生成 `simulation-with-simulide_v1.0.0.eext` 文件

2. **在 EasyEDA 中安装**

   - 打开 EasyEDA 专业版

   - 进入扩展管理器

   - 点击 "安装扩展"

   - 选择生成的 `.eext` 文件

   - 重启 EasyEDA

3. **使用扩展**

 - 在eda中新建仿真原理图

 - 即可使用


### 本地开发测试

#### 1. 启动本地测试服务器

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

#### WASM 模块 API

在独立应用中使用 WASM 模块：

```javascript

// 1. 导入 WASM 模块
import wasmModule from './wasm/lceda-pro-sim-server.js';

// 2. 初始化
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
const time = Module._getSimulationTime();  // 单位：皮秒 (ps)

// 9. 停止仿真
Module.ccall('stopSimulation', null, [], []);

```

#### EasyEDA 扩展 API

在 EasyEDA 扩展中使用平台 API：

```typescript

// 扩展激活入口

export function activate(status?: 'onStartupFinished', arg?: string): void {
    if (status === 'onStartupFinished') {

        // 注册仿真引擎事件监听器
        eda.sch_Event.addSimulationEnginePullEventListener(
            'sim-engine-monitor',
            'all',
            handleSimulationEvent
        );
    }
}

// 事件处理函数
async function handleSimulationEvent(
    eventType: SimEventType,
    props: any
): Promise<void> {
    switch (eventType) {
        case 'SESSION_START':
            // 加载电路并启动仿真
            await loadCircuit(props.fileContent);
            await startSimulation();
            break;
        case 'SESSION_PAUSE':
            await pauseSimulation();
            break;
        case 'SESSION_RESUME':
            await resumeSimulation();
            break;
        case 'SESSION_STOP':
            await stopSimulation();
            break;
        case 'SPEED_SET':
            setSimulationSpeed(props.speed);
            break;
        case 'COMPONENT_UPDATE':
            await updateComponent(props.id, props.attr, props.value);
            break;
    }
}

// 推送仿真数据到平台
function pushSimulationData(data: SimulationData): void {
    eda.sch_SimulationEngine.pushData(
        SimPushEventType.STREAM_DATA,
        JSON.stringify(data)
    );
}

// 获取扩展文件（如 WASM 文件）
async function getWasmFile(): Promise<File | undefined> {
    return eda.sys_FileSystem.getExtensionFile('/wasm/lceda-pro-sim-server.wasm');
}

```

## 🙏 致谢

  

- **SimulIDE 项目** ([simulide.com](https://simulide.com/p/)) - 感谢提供优秀的电路仿真引擎

  

- **Emscripten 团队** - 让 C++ 代码能够在浏览器中运行

  

- 所有项目贡献者和开源社区的支持

  

  

## 📚 相关资源

  
  

- [SimulIDE 官方网站](https://simulide.com/)

  

- [WebAssembly 官方文档](https://webassembly.org/)

  

- [Emscripten 文档](https://emscripten.org/docs/)

  
  

## 📄 许可证

  
  

本项目采用双许可证模式，详见 [LICENSE](LICENSE) 文件：

  

- 原始 SimulIDE 代码: AGPLv3

  

- 修改和新增代码: GPLv3