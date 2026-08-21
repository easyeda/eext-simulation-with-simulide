# CHANGELOG

> **eext-simulation-with-simulide** —— 基于 SimulIDE 的 WebAssembly 电路仿真引擎，为嘉立创EDA专业版 / EasyEDA 提供浏览器端实时电路仿真能力（C++20 编写，经 Emscripten 编译为 WASM，以扩展插件形式集成）。

## 1.8.1

- 修复多运放交叉耦合电路（如双运放 + BJT 恒流源）仿真报 `NonLinear Not Converging` 的问题。
- 修复加载部分含子电路的电路（如 74XX69、74HC40103 等）时仿真直接崩溃的问题。
- 修复芯片文件查找失败时仿真直接停止的问题。
- 修复数值单位解析不一致导致器件引脚数错误、数字电路仿真结果不正确的问题。
- 新增函数发生器（Function）与存储器（Memory）器件支持（用于特定芯片）。
- 提升运放、稳压器等元件的仿真收敛性与稳定性。
- 修复含子电路的数字 IC（如 74HC283、74XX82）加载时报 `null startPin/null endPin in Connector`、内部逻辑悬空的问题。
- 修复运放在高增益负反馈闭环电路中仿真报 `NonLinear Not Converging` 的问题。