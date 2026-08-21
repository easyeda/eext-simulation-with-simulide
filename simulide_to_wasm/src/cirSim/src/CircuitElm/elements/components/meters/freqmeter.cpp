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

#include "freqmeter.h"
#include "itemlibrary.h"
#include "simulator.h"
#include "pin.h"
#include "doubleProp.h"

Component* FreqMeter::construct(std::string type, std::string id)
{
    return new FreqMeter(type, id);
}

LibraryItem* FreqMeter::libraryItem()
{
    return new LibraryItem(
        "Frequency Meter",
        "Meters",
        "",
        "FreqMeter",
        FreqMeter::construct);
}

FreqMeter::FreqMeter(std::string type, std::string id)
    : Meter(type, id)
    , eElement(id)
    , m_inputPin(nullptr)
    , m_rising(false)
    , m_falling(false)
    , m_filter(0.1)
    , m_lastVoltage(0.0)
    , m_freq(0.0)
    , m_numMax(0)
    , m_lastMax(0)
    , m_totalPeriod(0)
    , m_period(0)
{
    // 创建输入引脚
    m_pin.resize(1);
    m_ePin.resize(1);
    m_pin[0] = new Pin(m_id + "-lpin", 0, this);
    m_ePin[0] = m_pin[0];

    // 添加到仿真器更新列表
    Simulator::self()->addToUpdateList(this);

    // 添加属性
    addPropGroup({"Main", {
        new doubleProp<FreqMeter>("Filter", "Filter", "V",this, &FreqMeter::filter, &FreqMeter::setFilter),
    }, 0});

    // 初始化
    initialize();
}

FreqMeter::~FreqMeter()
{
    // 清理数据
    for (auto& [id, ptr] : m_freqData) {
        delete ptr;
    }
    m_freqData.clear();
}

void FreqMeter::initialize()
{
    // 重置所有状态变量
    m_rising       = false;
    m_falling      = false;
    m_lastVoltage  = 0.0;
    m_freq         = 0.0;
    m_numMax       = 0;
    m_lastMax      = 0;
    m_totalPeriod  = 0;
    m_period       = 0;
}

void FreqMeter::stamp()
{
    // 仿真开始时调用，注册电压变化回调
    if (m_ePin[0]) {
        m_ePin[0]->changeCallBack(this, true);
    }
}

void FreqMeter::runEvent()
{
    // 元件事件处理（暂无使用）
}

void FreqMeter::timeStep(uint64_t currentTime)
{
    // 每次时间步计算频率
    calcFrequency();

    // 采样计数
    m_ctr++;
    if (m_ctr >= m_speed) {
        // 存储频率数据
        const std::string freqId = m_id + "-freq";
        if (m_freqData.find(freqId) == m_freqData.end()) {
            m_freqData[freqId] = new MeterData(freqId, m_length);
        }
        m_freqData[freqId]->datas.push(std::make_pair(currentTime, m_freq));
        m_ctr = 0;
    }
}

void FreqMeter::viewDate()
{
    // 视图数据（暂无使用）
}

void FreqMeter::voltChanged()
{
    // 电压变化时触发频率检测
    uint64_t simTime = Simulator::self()->circTime();
    double currentVoltage = m_ePin[0]->getVoltage();
    double delta = currentVoltage - m_lastVoltage;

    // 上升沿检测
    if (delta > m_filter) {
        if (m_falling && !m_rising) {
            m_falling = false; // 发现谷值点
        }
        m_rising = true;
        m_lastVoltage = currentVoltage;
    }
    // 下降沿检测
    else if (delta < -m_filter) {
        if (m_rising && !m_falling) {
            // 发现峰值点，记录周期
            if (m_numMax > 0) {
                m_period = simTime - m_lastMax;
                m_totalPeriod += m_period;
            }
            m_lastMax = simTime;
            m_numMax++;
            m_rising = false;
        }
        m_falling = true;
        m_lastVoltage = currentVoltage;
    }
}

void FreqMeter::calcFrequency()
{
    double freq = m_freq;

    if (m_period > 0) {
        uint64_t simTime = Simulator::self()->circTime();
        uint64_t psPerFrame = Simulator::self()->psPerSec();
        uint64_t lostTime = m_period * 2;

        // 确保丢失检测时间至少为每帧时间的两倍
        if (lostTime < psPerFrame * 2) {
            lostTime = psPerFrame * 2;
        }

        // 如果超过丢失时间，认为信号丢失
        if (simTime - m_lastMax > lostTime) {
            freq = 0.0;
            m_period = 0;
            m_totalPeriod = 0;
            m_numMax = 0;
        }
        else if (m_numMax > 1) {
            // 频率 = 1e12 / 平均周期（ps → Hz）
            freq = 1e12 / (static_cast<double>(m_totalPeriod) / static_cast<double>(m_numMax - 1));
            m_totalPeriod = 0;
            m_numMax = 0;
        }
    }

    // 更新频率值
    if (m_freq != freq) {
        m_freq = freq;
    }
}

std::unordered_map<std::string, MeterData*> FreqMeter::getDataImpl()
{
    return m_freqData;
}