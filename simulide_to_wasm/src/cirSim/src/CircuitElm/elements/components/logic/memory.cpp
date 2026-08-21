/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2026 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */

#include "memory.h"
#include "itemlibrary.h"
#include "iopin.h"
#include "simulator.h"

#include "intProp.h"
#include "boolProp.h"
#include "stringProp.h"

Component* Memory::construct( std::string type, std::string id )
{ return new Memory( type, id ); }

LibraryItem* Memory::libraryItem()
{
    return new LibraryItem(
        "Ram/Rom",
        "Memory",
        "2to3g.png",
        "Memory",
        Memory::construct );
}

Memory::Memory( std::string type, std::string id )
      : IoComponent( type, id )
      , eElement( id )
{
    m_width  = 4;
    m_height = 11;

    m_asynchro  = true;
    m_persistent = false;
    m_dataBytes = 1;
    m_addrBits  = 0;
    m_dataBits  = 0;
    m_address   = 0;

    m_oe = false;
    m_cs = true;
    m_we = true;

    // WE / CS / OE 控制脚（均低有效），放入 m_otherPin
    m_WePin = new IoPin( m_id+"-Pin_We", 0, this, input );
    setupPin( m_WePin, "L01WE" );
    m_WePin->setInverted( true );
    m_otherPin.emplace_back( m_WePin );

    m_CsPin = new IoPin( m_id+"-Pin_Cs", 0, this, input );
    setupPin( m_CsPin, "R01CS" );
    m_CsPin->setInverted( true );
    m_otherPin.emplace_back( m_CsPin );

    m_OePin = new IoPin( m_id+"-Pin_outEnable", 0, this, input );
    setupPin( m_OePin, "L02OE" );
    m_OePin->setInverted( true );
    m_otherPin.emplace_back( m_OePin );

    setAddrBits( 8 );     // 创建地址引脚（m_inpPin）
    setDataBits( 8 );     // 创建数据引脚（m_outPin）

    Simulator::self()->addToUpdateList( this );

    addPropGroup( { "Main", {
        new intProp <Memory>("Address_Bits", "Address Size","_bits"
                            , this, &Memory::addrBits, &Memory::setAddrBits, propNoCopy,"uint" ),

        new intProp <Memory>("Data_Bits", "Data Size","_bits"
                            , this, &Memory::dataBits, &Memory::setDataBits, propNoCopy,"uint" ),

        new boolProp<Memory>("Persistent", "Persistent",""
                            , this, &Memory::persistent, &Memory::setPersistent ),

        new boolProp<Memory>("Asynch", "Asynchronous",""
                            , this, &Memory::asynchro, &Memory::setAsynchro )
    }, groupNoCopy } );

    // Electric 组：合并 input/output 属性（cirSim 用 std::vector 手动拼接）
    std::vector<comProperty*> inputProps  = IoComponent::inputProps();
    std::vector<comProperty*> outputProps = IoComponent::outputProps();
    std::vector<comProperty*> outputType  = IoComponent::outputType();
    std::vector<comProperty*> electric;
    electric.reserve( inputProps.size()+outputProps.size()+outputType.size() );
    electric.insert( electric.end(), inputProps.begin(),  inputProps.end() );
    electric.insert( electric.end(), outputProps.begin(), outputProps.end() );
    electric.insert( electric.end(), outputType.begin(),  outputType.end() );
    addPropGroup( { "Electric", electric, 0 } );

    addPropGroup( { "Timing", IoComponent::edgeProps(), 0 } );

    // 隐藏的 Mem 属性：承载内存内容序列化，用于 .sim1 持久化（ROM）
    addPropGroup( { "Hidden", {
        new stringProp<Memory>("Mem","",""
                       , this, &Memory::getMem, &Memory::setMem )
    }, groupHidden } );
}
Memory::~Memory(){}

void Memory::stamp()                   // 仿真开始时调用
{
    m_oe = false;
    m_cs = true;
    m_we = true;

    for( IoPin* pin : m_outPin ) pin->setPinMode( input );
    IoComponent::initState();

    if( !m_persistent ) std::fill( m_ram.begin(), m_ram.end(), 0 );

    for( uint32_t i=0; i<m_inpPin.size(); ++i )
        m_inpPin[i]->changeCallBack( this, m_asynchro );

    m_WePin->changeCallBack( this );
    m_CsPin->changeCallBack( this );
    m_OePin->changeCallBack( this );
}

void Memory::updateStep()
{
    if( m_changed )
    {
        for( IoPin* pin : m_inpPin ) pin->changeCallBack( this, m_asynchro && m_cs );
        for( IoPin* pin : m_outPin ) pin->changeCallBack( this, m_asynchro && m_cs && m_we );
        m_changed = false;
    }
}

void Memory::voltChanged()        // 某个引脚状态变化时处理
{
    bool cs = m_CsPin->getInpState();
    bool we = m_WePin->getInpState();
    bool oe = cs && !we && m_OePin->getInpState(); // 仅当 OE & CS & 读时才使能输出缓冲
    if( m_oe != oe || m_cs != cs || m_we != we )
    {
        m_oe = oe;
        for( IoPin* pin : m_outPin )         // 使能/禁用输出缓冲
        {
            pin->setPinMode( oe ? output : input );
            if( m_asynchro ) pin->changeCallBack( this, cs && we );
        }
    }
    // 仅在异步模式或 CS/WE 发生变化时操作
    bool operate = m_asynchro || m_cs != cs || m_we != we;
    m_cs = cs;
    m_we = we;

    if( !operate || !m_cs ) return;          // 无事可做

    m_address = 0;
    for( int i=0; i<m_addrBits; ++i )        // 读取地址
    {
        if( m_inpPin[i]->getInpState() ) m_address += ( 1<<i );
    }
    if( m_we ){                              // 写
        int value = 0;
        for( uint32_t i=0; i<m_outPin.size(); ++i )
        {
            if( m_outPin[i]->getInpState() ) value += ( 1<<i );
        }
        m_ram[m_address] = value;
    }
    else{                                    // 读
        m_nextOutVal = m_ram[m_address];
        IoComponent::scheduleOutPuts( this );
}   }

void Memory::setAsynchro( bool a )
{
    m_asynchro = a;
    m_changed = true;
}

// 将内存内容序列化为逗号分隔的十进制串（去掉尾部连续的 0，末尾带逗号）。
// 与上游 SimulIDE 的 MemData::getMem 格式保持一致。
std::string Memory::getMem()
{
    std::string m;
    int size = m_ram.size();
    if( size > 0 )
    {
        bool empty = true;
        for( int i=size-1; i>=0; --i )
        {
            int val = m_ram[i];
            if( val != 0 ) empty = false;
            if( empty ) continue;
            m = std::to_string( val ) + "," + m;
    }   }
    return m;
}

// 从逗号分隔的十进制串恢复内存内容（空 token 视为 0）。
void Memory::setMem( std::string m )
{
    if( m.empty() ) return;

    size_t start = 0;
    size_t i = 0;
    while( true )
    {
        size_t end = m.find( ',', start );
        std::string tok = (end == std::string::npos) ? m.substr( start )
                                                     : m.substr( start, end-start );
        if( i < m_ram.size() )
        {
            m_ram[i] = tok.empty() ? 0 : std::stoi( tok );
            i++;
        }
        if( end == std::string::npos ) break;
        start = end + 1;
    }
}

void Memory::setAddrBits( int bits )
{
    if( bits == m_addrBits ) return;
    if( bits == 0 ) bits = 8;
    if( bits > 24 ) bits = 24;

    m_ram.resize( 1 << bits );                // 1..24 位 → 2..16M

    setNumInps( bits, "A" );                   // 地址引脚即 m_inpPin（动态增删由基类处理）
    m_addrBits = bits;

    if( !m_persistent ) std::fill( m_ram.begin(), m_ram.end(), 0 );
}

void Memory::setDataBits( int bits )
{
    if( bits == m_dataBits ) return;
    if( bits == 0 ) bits = 8;
    if( bits > 32 ) bits = 32;

    setNumOuts( bits, "D" );                   // 数据引脚即 m_outPin
    m_dataBits = bits;

    m_dataBytes = m_dataBits/8;
    if( m_dataBits%8 ) m_dataBytes++;
}
