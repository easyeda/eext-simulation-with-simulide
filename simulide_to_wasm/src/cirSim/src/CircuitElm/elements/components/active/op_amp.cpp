/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2025 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */


#include "op_amp.h"
#include "itemlibrary.h"
#include "connector.h"
#include "simulator.h"
#include "e-node.h"
#include "iopin.h"

#include "doubleProp.h"
#include "boolProp.h"

#define tr(str) simulideTr("OpAmp",str)

Component* OpAmp::construct( std::string type, std::string id )
{ return new OpAmp( type, id ); }

LibraryItem* OpAmp::libraryItem()
{
    return new LibraryItem(
        "OpAmp",
        "Other Active",
        "opamp.png",
        "opAmp",
        OpAmp::construct );
}

OpAmp::OpAmp( std::string type, std::string id )
     : Component( type, id )
     , eElement( id )
{
    m_pin.resize( 5 );
    m_pin[0] = m_inputP = new IoPin( id+"-inputNinv", 0, this, input );

    m_pin[1] = m_inputN = new IoPin(id+"-inputInv", 1, this, input );

    m_output = new IoPin(id+"-output",   2, this, source );
    m_pin[2] = m_output;

    m_pin[3] = new Pin( id+"-powerPos", 3, this );

    m_pin[4] = new Pin(id+"-powerNeg", 4, this );

    
    m_switchPins = false;
    setPowerPins( false );

    m_gain = 1000;
    m_outImp = cero_doub;
    m_voltPosDef = 5;
    m_voltNegDef = 0;

    addPropGroup( { "Main", {
new doubleProp<OpAmp>( "Gain"     , "Gain"            ,"" , this, &OpAmp::gain,   &OpAmp::setGain ),
new doubleProp<OpAmp>( "Out_Imped", "Output Impedance","Ω", this, &OpAmp::outImp, &OpAmp::setOutImp ),
    },0} );
    addPropGroup( { "Supply", {
new boolProp<OpAmp>("Power_Pins" , "Use Supply Pins"   ,"" , this, &OpAmp::powerPins,  &OpAmp::setPowerPins, propNoCopy ),
new boolProp<OpAmp>("Switch_Pins", "Switch Supply Pins","" , this, &OpAmp::switchPins, &OpAmp::setSwitchPins, propNoCopy ),
new doubleProp<OpAmp>("Volt_Pos"   , "V+"               ,"V", this, &OpAmp::voltPos,    &OpAmp::setVoltPos ),
new doubleProp<OpAmp>("Volt_Neg"   , "V-"               ,"V", this, &OpAmp::voltNeg,    &OpAmp::setVoltNeg ),
    },0} );
}
OpAmp::~OpAmp(){}

void OpAmp::initialize()
{
    added = false;
    m_lastOut = 0;
    m_lastIn  = 0;
    m_lastSlope = std::numeric_limits<double>::quiet_NaN();
    m_lastIntercept = std::numeric_limits<double>::quiet_NaN();
    m_lastRegion = -2;
    m_lastModelTime = 0;
    m_effectiveGain = std::copysign( std::min(std::fabs(m_gain), 1.0), m_gain );
}

void OpAmp::stamp()
{
    if( m_inputP->isConnected() ) m_inputP->getEnode()->addToNoLinList(this);
    if( m_inputN->isConnected() ) m_inputN->getEnode()->addToNoLinList(this);
    if( m_output->isConnected() )
    {
        m_output->getEnode()->addToNoLinList(this);
        m_output->createCurrent();
        m_output->setImp( m_outImp );

        const int positiveNode = m_inputP->isConnected()
                               ? m_inputP->getEnode()->getNodeNumber() : -1;
        const int negativeNode = m_inputN->isConnected()
                               ? m_inputN->getEnode()->getNodeNumber() : -1;
        if( positiveNode >= 0 ) m_output->addSingAdm( positiveNode, 0 );
        // addSingAdm/stampSingAdm 用输出引脚和目标节点标识矩阵项。
        // 两个输入指向同一节点时只创建一个聚合系数，避免无法区分的重复项。
        if( negativeNode >= 0 && negativeNode != positiveNode )
            m_output->addSingAdm( negativeNode, 0 );
    }
}

void OpAmp::updateStep()
{
    if( !m_changed ) return;
    m_changed = false;

    m_output->setImp( m_outImp );
    voltChanged();
}

void OpAmp::voltChanged() // 任一引脚节点电压改变时调用
{
    if( !m_inputP->isConnected() || !m_inputN->isConnected()
     || !m_output->isConnected() ) return;

    if( m_powerPins )
    {
        m_voltPos = m_pin[3]->getVoltage();
        m_voltNeg = m_pin[4]->getVoltage();
    } 
    else 
    {
        m_voltPos = m_voltPosDef;
        m_voltNeg = m_voltNegDef;
    }
    const double vd = m_inputP->getVoltage()-m_inputN->getVoltage();

    double slope = 0;
    double out = 0;
    double intercept = 0;
    int region = 0;
    auto updateModel = [&]()
    {
        const double target = vd*m_effectiveGain;
        slope = m_effectiveGain;
        out = target;
        region = 0;
        if( target > m_voltPos )
        {
            out = m_voltPos;
            slope = 0;
            region = 1;
        }
        else if( target < m_voltNeg )
        {
            out = m_voltNeg;
            slope = 0;
            region = -1;
        }
        intercept = out-slope*vd;
    };
    updateModel();

    // 正反馈比较器在新的瞬态时刻可能从饱和支路跨入线性区，
    // 满增益牛顿步会在两个工作区之间反复跳变。仅当新仿真时刻
    // 引起工作区变化时重启增益续接；同一时刻续接过程自身的换区必须放行。
    Simulator* simulator = Simulator::self();
    const uint64_t modelTime = simulator->circTime();
    if( modelTime != m_lastModelTime && m_lastRegion != -2
     && region != m_lastRegion
     && std::fabs(m_effectiveGain) >= std::fabs(m_gain) )
    {
        m_effectiveGain = std::copysign( std::min(std::fabs(m_gain), 1.0), m_gain );
        updateModel();
    }

    // 增益续接为连续求解提供邻近工作点。若饱和区产生相同矩阵印章，
    // 可直接跳过这些续接值，直到模型变化或达到设定增益。
    while( slope == m_lastSlope && intercept == m_lastIntercept
        && std::fabs(m_effectiveGain) < std::fabs(m_gain) )
    {
        const double nextMagnitude = std::min( std::fabs(m_gain),
                                                std::fabs(m_effectiveGain)*1.5 );
        m_effectiveGain = std::copysign( nextMagnitude, m_gain );
        updateModel();
    }

    // 分段线性牛顿形式：Vout = slope*(V+ - V-) + intercept。
    // 斜率和截距未变化时，当前矩阵已精确表示该工作区。
    if( slope == m_lastSlope && intercept == m_lastIntercept ) return;

    if( simulator->convergenceDiagnosticsEnabled() )
    {
        std::ostringstream convergenceState;
        convergenceState << "vd=" << vd
                         << " targetOut=" << out
                         << " previousIn=" << m_lastIn
                         << " previousOut=" << m_lastOut
                         << " slope=" << slope
                         << " rails=[" << m_voltNeg << ',' << m_voltPos << ']';
        simulator->notCorverged( m_elmId, convergenceState.str() );
    }
    else simulator->notCorverged();

    const int positiveNode = m_inputP->getEnode()->getNodeNumber();
    const int negativeNode = m_inputN->getEnode()->getNodeNumber();
    const double outputAdmitance = 1/m_outImp;
    if( positiveNode == negativeNode )
        m_output->stampSingAdm( positiveNode, 0 );
    else
    {
        m_output->stampSingAdm( positiveNode,  outputAdmitance*slope );
        m_output->stampSingAdm( negativeNode, -outputAdmitance*slope );
    }
    m_output->stampCurrent( outputAdmitance*intercept );

    m_lastSlope = slope;
    m_lastIntercept = intercept;
    m_lastRegion = region;
    m_lastModelTime = modelTime;
    m_lastIn  = vd;
    m_lastOut = out;
}

void OpAmp::setGain( double gain )
{
    m_gain = gain;
    m_effectiveGain = std::copysign( std::min(std::fabs(m_gain), 1.0), m_gain );
    m_lastSlope = std::numeric_limits<double>::quiet_NaN();
    m_lastIntercept = std::numeric_limits<double>::quiet_NaN();
    m_lastRegion = -2;
    m_lastModelTime = 0;
    m_changed = true;
}

void OpAmp::setOutImp( double imp )
{
    if( imp < cero_doub ) imp = cero_doub;
    m_outImp = imp;
    m_lastSlope = std::numeric_limits<double>::quiet_NaN();
    m_lastIntercept = std::numeric_limits<double>::quiet_NaN();
    m_changed = true;
    if( !Simulator::self()->isRunning() ) updateStep();
}

void OpAmp::setPowerPins( bool set )
{
    //m_pin[3]->setEnabled( set );
    //m_pin[3]->setVisible( set );
    //m_pin[4]->setEnabled( set );
    //m_pin[4]->setVisible( set );
    
    if( !set )
    {
        m_pin[3]->removeConnector();
        m_pin[4]->removeConnector();
    }
    m_powerPins = set;
    udtProperties();
}

void OpAmp::setSwitchPins( bool s )
{
    m_switchPins = s;
    int angleP = s ? 270 : 90;
    int angleN = s ? 90  : 270;
    //qreal yP   = s ?  16 : -16;
    //qreal yN   = s ? -16 : 16;
    //m_pin[3]->setPinAngle( angleP );
    //m_pin[3]->setY( yP );
    //m_pin[4]->setPinAngle( angleN );
    //m_pin[4]->setY( yN );
}

void OpAmp::udtProperties()
{
    //if( !m_propDialog ) return;
    //m_propDialog->showProp("Volt_Pos", !m_powerPins );
    //m_propDialog->showProp("Volt_Neg", !m_powerPins );
    //m_propDialog->showProp("Switch_Pins", m_powerPins );
}
