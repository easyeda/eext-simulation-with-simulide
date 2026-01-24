#include "circuit.h"
#include "simulator.h"
#include "itemlibrary.h"
#include "meter.h"
#include "pch.h"
#include <cstdio>
#include <cstring>


#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;
#endif

// 全局变量
Circuit* g_circuit = nullptr;
Simulator* g_simulator = nullptr;

// 更新数据位置
std::unordered_map<std::string, size_t> lastReadPosMap;

bool startsWithWire(const std::string &str) {
    return str.compare(0, 4, "Wire") == 0;
}

#ifdef __EMSCRIPTEN__
static void logInfo(const char* msg) {
    emscripten_log(EM_LOG_CONSOLE, "%s", msg);
}

static void logError(const char* msg) {
    emscripten_log(EM_LOG_ERROR, "%s", msg);
}
#else
static void logInfo(const char* msg) {
    std::printf("[simulide][info] %s\n", msg);
}

static void logError(const char* msg) {
    std::fprintf(stderr, "[simulide][error] %s\n", msg);
}
#endif

#ifdef __EMSCRIPTEN__
extern "C" {
#endif

// 导入网表，并新建仿真实例
EMSCRIPTEN_KEEPALIVE
int loadCircuitFromFile(const char* filename, const char* fileContent) {
    try {
        if (!filename) {
            logError("loadCircuitFromFile: filename is null");
        }
        if (!fileContent) {
            logError("loadCircuitFromFile: fileContent is null");
            return -1; // Error: null content
        }
        std::printf("[simulide][info] loadCircuitFromFile: filename=%s, size=%zu\n",
            filename ? filename : "(null)", std::strlen(fileContent));
        g_circuit = new Circuit(std::string(fileContent));
        g_simulator = g_circuit->m_simulator; 
        logInfo("loadCircuitFromFile: circuit created");

        return 0; // Success
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[simulide][error] loadCircuitFromFile exception: %s\n", e.what());
        return -2; // Error: exception during load
    } catch (...) {
        logError("loadCircuitFromFile: unknown exception");
        return -2;
    }
}

// 开始仿真
EMSCRIPTEN_KEEPALIVE
void startSimulation() {
    if (g_simulator) {
        logInfo("startSimulation");
        g_simulator->startSim();
    } else {
        logError("startSimulation: g_simulator is null");
    }
}

// 停止仿真
EMSCRIPTEN_KEEPALIVE
void stopSimulation() {
    if (g_simulator) {
        logInfo("stopSimulation");
        g_simulator->stopSim();
    } else {
        logError("stopSimulation: g_simulator is null");
    }
}

// 暂停仿真
EMSCRIPTEN_KEEPALIVE
void pauseSimulation() {
    if (g_simulator) {
        logInfo("pauseSimulation");
        g_simulator->pauseSim();
    } else {
        logError("pauseSimulation: g_simulator is null");
    }
}

// 再启动仿真
EMSCRIPTEN_KEEPALIVE
void resumeSimulation() {
    if (g_simulator) {
        logInfo("resumeSimulation");
        g_simulator->resumeSim();
    } else {
        logError("resumeSimulation: g_simulator is null");
    }
}

// 获取仿真状态
EMSCRIPTEN_KEEPALIVE
int getSimulationState() {
    if (g_simulator) {
        const int state = g_simulator->simState();
        return state;
    }
    logError("getSimulationState: g_simulator is null");
    return 0; 
}

// 设置当前仿真速度
EMSCRIPTEN_KEEPALIVE
void setSimulationSpeed(int speed) {
    if (g_simulator) {
        std::printf("[simulide][info] setSimulationSpeed: %d\n", speed);
        g_simulator->setSpeed(speed);
    } else {
        logError("setSimulationSpeed: g_simulator is null");
    }
}

// 获取当前仿真时间
EMSCRIPTEN_KEEPALIVE
double getSimulationTime() {
    if (g_simulator) {
        return g_simulator->circTime();
    }
    logError("getSimulationTime: g_simulator is null");
    return 0.0;
}

// 仿真步进函数
EMSCRIPTEN_KEEPALIVE
void stepSimulation() {
    if (g_simulator && g_simulator->simState() == SIM_RUNNING) {
        g_simulator->timerEvent();
    } else if (!g_simulator) {
        logError("stepSimulation: g_simulator is null");
    }
}

EMSCRIPTEN_KEEPALIVE 
const bool  updateCircuitData(const char* updateCirID ,const char* attrInput ,const char* updateValueStr){
    if (!g_circuit) {
        logError("updateCircuitData: g_circuit is null");
        return false;
    }
    if (!updateCirID || !attrInput || !updateValueStr) {
        logError("updateCircuitData: null input");
        return false;
    }
    std::printf("[simulide][info] updateCircuitData: id=%s attr=%s value=%s\n", updateCirID, attrInput, updateValueStr);
    std::vector<std::string> data{std::string(updateCirID),std::string(attrInput),std::string(updateValueStr)};
    bool result = g_circuit->upDateCmp(data);
    return result;
}

// 仿真数据获取函数
EMSCRIPTEN_KEEPALIVE
const char* getSimulationData() {
    if (!g_simulator) {
        logError("getSimulationData: g_simulator is null");
        static std::string empty = "{}";
        return empty.c_str();
    }

    try {
        auto outputData = g_simulator->getOutputData();
        std::ostringstream json;

        json << "{";
        bool firstEntry = true;

        if (outputData.meterData && !outputData.meterData->empty()) {
            for (const auto* meter : *outputData.meterData) {
                if (!meter) continue;

                auto& pos = lastReadPosMap[meter->id];

                auto dataVec = meter->datas.getDelta(pos);

                if (dataVec.empty()) continue;

                if (!firstEntry) json << ",";
                firstEntry = false;

                json << "\"" << meter->id << "\":";

                if (startsWithWire(meter->id)) {
                    const auto& [timestamp, value] = dataVec.back();
                    json << "[[" << timestamp << "," << value << "]]";
                } else {
                    json << "[";
                    bool firstData = true;
                    for (const auto& [timestamp, value] : dataVec) {
                        if (!firstData) json << ",";
                        firstData = false;
                        json << "[" << timestamp << "," << value << "]";
                    }
                    json << "]";
                }

            }
        }

        if (outputData.statusData && !outputData.statusData->empty()) {
            for (const auto* status : *outputData.statusData) {
                if (!status) continue;

                auto& pos = lastReadPosMap[status->id];

                auto dataVec = status->datas.getDelta(pos);

                if (dataVec.empty()) continue;

                if (!firstEntry) json << ",";
                firstEntry = false;

                json << "\"" << status->id << "\":";

                const auto& [timestamp, value] = dataVec.back();
                json << "[[" << timestamp << ",\"" << value << "\"]]";
            }
        }

        json << "}";

        static std::string result;
        result = json.str();
        return result.c_str();

    } catch (const std::exception& e) {
        std::fprintf(stderr, "[simulide][error] getSimulationData exception: %s\n", e.what());
        static std::string error = "{\"error\":\"Failed to get simulation data\"}";
        return error.c_str();
    } catch (...) {
        logError("getSimulationData: unknown exception");
        static std::string error = "{\"error\":\"Failed to get simulation data\"}";
        return error.c_str();
    }
}

// // 测试导出函数
// EMSCRIPTEN_KEEPALIVE
// int add(int a, int b) {
//     return a + b;
// }

// 获取版本
EMSCRIPTEN_KEEPALIVE
const char* getVersion() {
    return "EasyEDA Simulation Engine v1.0 (WASM)";
}

#ifdef __EMSCRIPTEN__
}
#endif

int main(){
    return 0;
}


