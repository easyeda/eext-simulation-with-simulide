/***************************************************************************
 *   Modified (C) 2025 by EasyEDA & JLC Technology Group                      *
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
#include "component.h"

class LibraryItem;
class eNode;


class Probe:public Meter
{
private:
    std::unordered_map<std::string,MeterData*> probes; //行是示波器个数，列是采样点个数，以后可以用变量来定义
    std::string m_voltMes;
    // 探针缓存
    std::vector<std::pair<eNode*, MeterData*>> m_watchCache;
    bool m_watchBuilt = false;
    uint64_t m_watchGen = 0;
public:
    Probe(std::string type,std::string id);
    ~Probe();

    static Component* construct(std::string type, std::string id);
    static LibraryItem* libraryItem();

    virtual void timeStep(uint64_t currentTime) override;
    virtual void viewDate() override;
    virtual std::unordered_map<std::string,MeterData*> getDataImpl() override;

    std::string voltMes(){ return m_voltMes; }
    void setVoltMes( std::string voltMes);

};


