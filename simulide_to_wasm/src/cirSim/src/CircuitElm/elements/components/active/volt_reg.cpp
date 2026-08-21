/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2025 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */

#include "volt_reg.h"
#include "connector.h"
#include "simulator.h"
#include "itemlibrary.h"
#include "pin.h"
#include "e-node.h"

#include "doubleProp.h"

#define tr(str) simulideTr("VoltReg",str)

Component* VoltReg::construct( std::string type, std::string id )
{ return new VoltReg( type, id ); }

LibraryItem* VoltReg::libraryItem()
{
    return new LibraryItem(
        "Volt. Regulator",
        "Other Active",
        "voltreg.png",
        "VoltReg",
        VoltReg::construct );
}

VoltReg::VoltReg( std::string type, std::string id )
       : Component( type, id )
       , eResistor( id )
{
    //m_area = QRect( -11, -8, 22, 19 );

    m_admit = 1e6;

    //setValLabelPos( 15, 12, 0 );
    
    m_pin.resize( 3 );
    m_ePin.resize( 3 );

    m_ePin[0] = m_pin[0] = new Pin(id+"-input", 0, this);
    //m_pin[0]->setLabelText( "I" );


    m_ePin[1] = m_pin[1] = new Pin(id+"-output", 1, this);
    //m_pin[1]->setLabelText( "O" );


    m_ePin[2] = m_pin[2] = new Pin( id+"-ref", 2, this );
    //m_pin[2]->setLength(5);
    //m_pin[2]->setLabelText( "R" );

    Simulator::self()->addToUpdateList( this );

    addPropGroup( { "Main", {
new doubleProp<VoltReg>("Voltage", "Output Voltage","V", this, &VoltReg::outVolt, &VoltReg::setOutVolt )
    },0} );

    //setShowProp("Voltage");
    setPropStr( "Voltage", "1.2" );
}
VoltReg::~VoltReg(){}

void VoltReg::stamp()
{
    m_connected = m_ePin[0]->isConnected()
               && m_ePin[1]->isConnected()
               && m_ePin[2]->isConnected();

    if( m_connected )
    {
        m_ePin[0]->getEnode()->addToNoLinList(this);
        m_ePin[1]->getEnode()->addToNoLinList(this);
        m_ePin[2]->getEnode()->addToNoLinList(this);

        m_ePin[0]->createCurrent();
        m_ePin[1]->createCurrent();

        const int inputNode = m_ePin[0]->getEnode()->getNodeNumber();
        const int refNode = m_ePin[2]->getEnode()->getNodeNumber();
        m_ePin[0]->addSingAdm( inputNode, 0 );
        m_ePin[1]->addSingAdm( inputNode, 0 );
        if( refNode != inputNode )
        {
            m_ePin[0]->addSingAdm( refNode, 0 );
            m_ePin[1]->addSingAdm( refNode, 0 );
        }
    }
    eResistor::stamp();
    m_lastInputSlope = std::numeric_limits<double>::quiet_NaN();
    m_lastRefSlope = std::numeric_limits<double>::quiet_NaN();
    m_lastOffset = std::numeric_limits<double>::quiet_NaN();
}

void VoltReg::updateStep()
{
    if( !m_changed ) return;
    m_changed = false;

    if( m_connected ) voltChanged();
}

void VoltReg::voltChanged()
{
    const double inputVolt = m_ePin[0]->getVoltage();
    const double refVolt = m_ePin[2]->getVoltage();
    double modelInput = inputVolt;
    if( modelInput < 1e-6 ) modelInput = 0;

    // 原模型的等效电流是分段仿射函数：
    // I = inputSlope*Vin + refSlope*Vref + offset。
    // 把两个斜率作为 Jacobian 写入矩阵后，每个工作区都能一次解出，
    // 不再用 1e6 倍电流源对参考端做不稳定的固定点迭代。
    double inputSlope = 0;
    double refSlope = 0;
    double offset = 0;
    const double delta = modelInput-(refVolt+m_vRef);
    if( delta >= 0.7 )
    {
        inputSlope = m_admit;
        refSlope = -m_admit;
        offset = -m_admit*m_vRef;
    }
    else if( modelInput >= 0.7 ) offset = 0.7*m_admit;
    else if( inputVolt >= 1e-6 ) inputSlope = m_admit;

    if( inputSlope == m_lastInputSlope
     && refSlope == m_lastRefSlope
     && offset == m_lastOffset ) return;

    m_lastInputSlope = inputSlope;
    m_lastRefSlope = refSlope;
    m_lastOffset = offset;
    Simulator::self()->notCorverged();

    const int inputNode = m_ePin[0]->getEnode()->getNodeNumber();
    const int refNode = m_ePin[2]->getEnode()->getNodeNumber();
    const bool commonControlNode = inputNode == refNode;
    const double combinedSlope = inputSlope+refSlope;

    m_ePin[0]->stampSingAdm( inputNode,
                             commonControlNode ? combinedSlope : inputSlope );
    m_ePin[1]->stampSingAdm( inputNode,
                             commonControlNode ? -combinedSlope : -inputSlope );
    if( !commonControlNode )
    {
        m_ePin[0]->stampSingAdm( refNode, refSlope );
        m_ePin[1]->stampSingAdm( refNode, -refSlope );
    }
    m_pin[0]->stampCurrent( offset );
    m_pin[1]->stampCurrent(-offset );
}
