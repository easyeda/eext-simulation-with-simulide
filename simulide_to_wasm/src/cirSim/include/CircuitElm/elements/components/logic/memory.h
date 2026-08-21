/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2026 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */

#pragma once

#include "pch.h"
#include "iocomponent.h"
#include "e-element.h"

class LibraryItem;

// 并行 RAM/ROM 存储器件
// 由上游 SimulIDE 的 Memory 移植而来，去掉 GUI（内存表/右键菜单）与 MemData 依赖，
// 自带 m_ram 并自行实现 setMem/getMem 持久化。地址位宽 1–24，数据位宽 1–32，
// WE/CS/OE 低有效控制，支持同步/异步模式与持久化（Persistent=ROM，否则 RAM 掉电清零）。
class Memory : public IoComponent, public eElement
{
    public:
        Memory( std::string type, std::string id );
        ~Memory();

        static Component* construct( std::string type, std::string id );
        static LibraryItem* libraryItem();

        virtual void stamp() override;
        virtual void updateStep() override;
        virtual void voltChanged() override;
        virtual void runEvent() override { IoComponent::runOutputs(); }

        void setMem( std::string m );
        std::string getMem();

        int addrBits() { return m_addrBits; }
        void setAddrBits( int bits );

        int dataBits() { return m_dataBits; }
        void setDataBits( int bits );

        bool persistent() { return m_persistent; }
        void setPersistent( bool p ) { m_persistent = p; }

        bool asynchro() { return m_asynchro; }
        void setAsynchro( bool a );

    private:
        int m_addrBits;     // 地址位宽
        int m_dataBits;     // 数据位宽
        int m_dataBytes;    // 每个 word 的字节数
        int m_address;      // 当前地址

        std::vector<int> m_ram;  // 存储内容

        bool m_oe;          // 输出使能（内部状态）
        bool m_we;          // 写使能（内部状态）
        bool m_cs;          // 片选（内部状态）
        bool m_persistent;  // 持久化标志（ROM）
        bool m_asynchro;    // 异步模式

        IoPin* m_CsPin;     // 片选脚
        IoPin* m_WePin;     // 写使能脚
        IoPin* m_OePin;     // 输出使能脚
};
