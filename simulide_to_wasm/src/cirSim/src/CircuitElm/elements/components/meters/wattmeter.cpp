/***************************************************************************
 *   Modified (C) 2026 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU Affero General Public License as        *
 *   published by the Free Software Foundation, either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU Affero General Public License for more details.                   *
 *                                                                         *
 *   You should have received a copy of the GNU Affero General Public      *
 *   License along with this program. If not, see                          *
 *   <http://www.gnu.org/licenses/>.                                       *
 ***************************************************************************/

#include "wattmeter.h"
#include "itemlibrary.h"
#include "simulator.h"
#include "pin.h"

Component* WattMeter::construct(std::string type, std::string id)
{
    return new WattMeter(type, id);
}

LibraryItem* WattMeter::libraryItem()
{
    return new LibraryItem(
        "Watt Meter",
        "Meters",
        "",
        "WattMeter",
        WattMeter::construct);
}

WattMeter::WattMeter(std::string type, std::string id)
    : Meter(type, id)
    , eResistor(id)
    , m_power(0.0)
    , m_voltage(0.0)
    , m_current(0.0)
{
    // 创建4个引脚
    // 引脚0,1: 电流测量端（串联）
    // 引脚2,3: 电压测量端（并联，高阻抗）
    m_pin.resize(4);
    m_ePin.resize(4);

    m_pin[PIN_CURRENT_IN] = new Pin(m_id + "-Iplus", PIN_CURRENT_IN, this);
    m_ePin[PIN_CURRENT_IN] = m_pin[PIN_CURRENT_IN];

    m_pin[PIN_CURRENT_OUT] = new Pin(m_id + "-Iminus", PIN_CURRENT_OUT, this);
    m_ePin[PIN_CURRENT_OUT] = m_pin[PIN_CURRENT_OUT];

    m_pin[PIN_VOLTAGE_P] = new Pin(m_id + "-Vplus", PIN_VOLTAGE_P, this);
    m_ePin[PIN_VOLTAGE_P] = m_pin[PIN_VOLTAGE_P];

    m_pin[PIN_VOLTAGE_N] = new Pin(m_id + "-Vminus", PIN_VOLTAGE_N, this);
    m_ePin[PIN_VOLTAGE_N] = m_pin[PIN_VOLTAGE_N];

    // 设置分流电阻为极小值，近似理想电流通路
    setRes(1e-9); // 1nΩ

    // 添加到仿真器更新列表
    Simulator::self()->addToUpdateList(this);

    // 初始化
    initialize();
}

WattMeter::~WattMeter()
{
    // 清理数据
    for (auto& [id, ptr] : m_powerData) {
        delete ptr;
    }
    m_powerData.clear();
}

void WattMeter::initialize()
{
    // 重置所有状态变量
    m_power = 0.0;
    m_voltage = 0.0;
    m_current = 0.0;
}

void WattMeter::stamp()
{
    // 调用 eResistor 的 stamp 方法
    eResistor::stamp();
}

void WattMeter::runEvent()
{
    // 元件事件处理（暂无使用）
}

void WattMeter::timeStep(uint64_t currentTime)
{
    // 测量电压：引脚2和引脚3之间的电压差
    if (m_ePin[PIN_VOLTAGE_P] && m_ePin[PIN_VOLTAGE_N]) {
        m_voltage = m_ePin[PIN_VOLTAGE_P]->getVoltage() - m_ePin[PIN_VOLTAGE_N]->getVoltage();
    }

    // 测量电流：通过 eResistor current() 方法
    m_current = current();

    // 计算功率 P = V × I
    m_power = m_voltage * m_current;

    // 采样计数
    m_ctr++;
    if (m_ctr >= m_speed) {
        // 存储功率数据
        const std::string powerId = m_id + "-power";
        if (m_powerData.find(powerId) == m_powerData.end()) {
            m_powerData[powerId] = new MeterData(powerId, m_length);
        }
        m_powerData[powerId]->datas.push(std::make_pair(currentTime, m_power));
        m_ctr = 0;
    }
}

void WattMeter::viewDate()
{
    // 视图数据（暂无使用）
}

std::unordered_map<std::string, MeterData*> WattMeter::getDataImpl()
{
    return m_powerData;
}
