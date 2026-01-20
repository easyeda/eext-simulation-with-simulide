#include "circuit.h"
#include "simulator.h"
#include "itemlibrary.h"
#include "meter.h"
#include "pch.h"


#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;
#endif

// Global circuit instance
Circuit* g_circuit = nullptr;
Simulator* g_simulator = nullptr;

// 更新数据位置
std::unordered_map<std::string, size_t> lastReadPosMap;

bool startsWithWire(const std::string &str) {
    return str.compare(0, 4, "Wire") == 0;
}

#ifdef __EMSCRIPTEN__
extern "C" {
#endif

// 导入网表，并新建仿真实例
EMSCRIPTEN_KEEPALIVE
int loadCircuitFromFile(const char* filename, const char* fileContent) {
    try {
        if (!fileContent) {
            return -1; // Error: null content
        }
        g_circuit = new Circuit(std::string(fileContent));
        g_simulator = g_circuit->m_simulator; 

        return 0; // Success
    } catch (const std::exception& e) {
        return -2; // Error: exception during load
    }
}

// 开始仿真
EMSCRIPTEN_KEEPALIVE
void startSimulation() {
    if (g_simulator) {
        g_simulator->startSim();
    }
}

// 停止仿真
EMSCRIPTEN_KEEPALIVE
void stopSimulation() {
    if (g_simulator) {
        g_simulator->stopSim();
    }
}

// 暂停仿真
EMSCRIPTEN_KEEPALIVE
void pauseSimulation() {
    if (g_simulator) {
        g_simulator->pauseSim();
    }
}

// 再启动仿真
EMSCRIPTEN_KEEPALIVE
void resumeSimulation() {
    if (g_simulator) {
        g_simulator->resumeSim();
    }
}

// 获取仿真状态
EMSCRIPTEN_KEEPALIVE
int getSimulationState() {
    if (g_simulator) {
        return g_simulator->simState();
    }
    return 0; // SIM_STOPPED
}

// 设置当前仿真速度
EMSCRIPTEN_KEEPALIVE
void setSimulationSpeed(int speed) {
    if (g_simulator) {
        g_simulator->setSpeed(speed);
    }
}

// 获取当前仿真时间
EMSCRIPTEN_KEEPALIVE
double getSimulationTime() {
    if (g_simulator) {
        return g_simulator->circTime();
    }
    return 0.0;
}

// 仿真步进函数
EMSCRIPTEN_KEEPALIVE
void stepSimulation() {
    if (g_simulator && g_simulator->simState() == SIM_RUNNING) {
        g_simulator->timerEvent();
    }
}

EMSCRIPTEN_KEEPALIVE 
const bool  updateCircuitData(const char* updateCirID ,const char* attrInput ,const char* updateValueStr){
    std::vector<std::string> data{std::string(updateCirID),std::string(attrInput),std::string(updateValueStr)};
    bool result = g_circuit->upDateCmp(data);
    return result;
}

// 仿真数据获取函数
EMSCRIPTEN_KEEPALIVE
const char* getSimulationData() {
    if (!g_simulator) {
        static std::string empty = "{}";
        return empty.c_str();
    }

    try {
        auto outputData = g_simulator->getOutputData();
        std::ostringstream json;

        json << "{\"meters\":[";

        if (outputData.meterData && !outputData.meterData->empty()) {
            bool firstMeter = true;
            for (const auto* meter : *outputData.meterData) {
                if (!meter) continue;

                auto& pos = lastReadPosMap[meter->id];

                auto dataVec = meter->datas.getDelta(pos);

                if (dataVec.empty()) continue;

                if (!firstMeter) json << ",";
                firstMeter = false;

                json << "{\"id\":\"" << meter->id << "\",\"data\":";

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

                json << "}";
            }
        }

        json << "],\"status\":[";

        if (outputData.statusData && !outputData.statusData->empty()) {
            bool firstStatus = true;
            for (const auto* status : *outputData.statusData) {
                if (!status) continue;

                auto& pos = lastReadPosMap[status->id];

                auto dataVec = status->datas.getDelta(pos);

                if (dataVec.empty()) continue;

                if (!firstStatus) json << ",";
                firstStatus = false;

                json << "{\"id\":\"" << status->id << "\",\"data\":";

                const auto& [timestamp, value] = dataVec.back();
                json << "[[" << timestamp << ",\"" << value << "\"]]";

                json << "}";
            }
        }

        json << "]}";

        static std::string result;
        result = json.str();
        return result.c_str();

    } catch (const std::exception& e) {
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


