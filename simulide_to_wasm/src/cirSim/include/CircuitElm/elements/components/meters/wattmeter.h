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
#include "e-resistor.h"

class LibraryItem;

class WattMeter : public Meter, public eResistor
{
public:
    WattMeter(std::string type, std::string id);
    ~WattMeter();

    static Component* construct(std::string type, std::string id);
    static LibraryItem* libraryItem();

    // eElement 接口
    void initialize() override;
    void stamp() override;
    void runEvent() override;

    // Meter 接口
    void timeStep(uint64_t currentTime) override;
    void viewDate() override;
    std::unordered_map<std::string, MeterData*> getDataImpl() override;

    // 获取当前功率值
    double getPower() const { return m_power; }
    // 获取电压值
    double getVoltage() const { return m_voltage; }
    // 获取电流值
    double getCurrent() const { return m_current; }

private:
    // 功率数据存储
    std::unordered_map<std::string, MeterData*> m_powerData;

    // 测量值
    double m_power;    // 功率 (W)
    double m_voltage;  // 电压 (V)
    double m_current;  // 电流 (A)

    // 引脚索引
    static const int PIN_CURRENT_IN = 0;   // 电流流入引脚 (I+)
    static const int PIN_CURRENT_OUT = 1;  // 电流流出引脚 (I-)
    static const int PIN_VOLTAGE_P = 2;    // 电压正极引脚 (V+)
    static const int PIN_VOLTAGE_N = 3;    // 电压负极引脚 (V-)
};
