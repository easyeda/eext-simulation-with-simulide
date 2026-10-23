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

#include  "ammeter.h"
#include "itemlibrary.h"
#include "simulator.h"

Component *Ammeter::construct(std::string type, std::string id)
{
    return new Ammeter(type,id);
}

LibraryItem *Ammeter::libraryItem()
{
       return new LibraryItem(
        "Ammeter",
        "Meters",
        " ",
        "Ammeter",
        Ammeter::construct);
}

Ammeter::Ammeter(std::string type,std::string id)
       :Meter(type,id)
{
    currentsData.clear();
}

Ammeter::~Ammeter()
{
    for (auto& [id, ptr] : currentsData) {
        delete ptr;
    }
    currentsData.clear();
}


void Ammeter::timeStep(uint64_t currentTime)
{
    //非电流模式，不进入数据提取
    if(!Simulator::self()->getCurrentModel()) return;

    m_ctr++;
    if (m_ctr < m_speed) return;

    // 缓存失效条件: 节点列表重建(新电路)或元件列表规模变化(增删元件)
    const std::vector<eElement*>& elements = m_cirSim->getElementList();
    if( !m_wireBuilt || m_wireGen != m_cirSim->nodeListGen() || m_wireElemCount != elements.size() )
    {
        m_wireCache.clear();
        for( eElement* Element : elements )
        {
            const std::string Id = Element->getId();
            if( (Id.find("Wire") == std::string::npos) || (Id.find("outnod") != std::string::npos) ) continue;
            auto it = currentsData.find(Id);
            if( it == currentsData.end() ){
                it = currentsData.emplace( Id, new MeterData(Id, m_length) ).first;
            }
            m_wireCache.push_back({ Element, it->second });
        }
        m_wireGen       = m_cirSim->nodeListGen();
        m_wireElemCount = elements.size();
        m_wireBuilt     = true;
    }
    // 仅向缓存的Wire采样,不再全元件扫描
    for( auto& wire : m_wireCache )
        wire.second->datas.push( std::make_pair(currentTime, wire.first->getElementData()) );
    m_ctr = 1;
}

void Ammeter::viewDate()
{
}

std::unordered_map<std::string,MeterData*>  Ammeter::getDataImpl()
{
    return currentsData;
}

