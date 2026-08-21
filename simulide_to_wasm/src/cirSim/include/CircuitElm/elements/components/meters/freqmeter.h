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

#pragma once

#include "meter.h"
#include "e-element.h"
#include "e-pin.h"

class LibraryItem;

class FreqMeter : public Meter, public eElement
{
public:
    FreqMeter(std::string type, std::string id);
    ~FreqMeter();

    static Component* construct(std::string type, std::string id);
    static LibraryItem* libraryItem();

    // eElement 接口
    void initialize() override;
    void stamp() override;
    void runEvent() override;
    void voltChanged() override;

    // Meter 接口
    void timeStep(uint64_t currentTime) override;
    void viewDate() override;
    std::unordered_map<std::string, MeterData*> getDataImpl() override;

    // 属性访问
    double filter() { return m_filter; }
    void setFilter(double f) { m_filter = f; }
    double getFrequency() const { return m_freq; }


private:
    void calcFrequency(); // 计算频率值

    // 引脚管理
    Pin* m_inputPin;     // 输入引脚

    // 频率检测算法变量
    bool    m_rising;       // 上升沿标志
    bool    m_falling;      // 下降沿标志
    double  m_filter;       // 电压变化滤波阈值（V）
    double  m_lastVoltage;  // 上一次电压值

    double  m_freq;         // 当前频率值（Hz）
    int     m_numMax;       // 检测到的峰/谷点数量

    uint64_t m_lastMax;     // 上一个峰/谷点时间（ps）
    uint64_t m_totalPeriod; // 累计周期总和（ps）
    uint64_t m_period;      // 当前周期（ps）

    // 数据输出
    std::unordered_map<std::string, MeterData*> m_freqData;
};
