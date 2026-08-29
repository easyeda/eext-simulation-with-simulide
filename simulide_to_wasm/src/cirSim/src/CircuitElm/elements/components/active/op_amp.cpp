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

#include <algorithm>
#include <cmath>
#include <sstream>

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

    
    m_powerPins = false;
    m_switchPins = false;
    m_gain = 1000;
    m_outImp = cero_doub;
    m_voltPosDef = 5;
    m_voltNegDef = 0;
    resetSolverState();
    setPowerPins( false );

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

bool OpAmp::sameEquation( const Linearization& lhs,
                          const Linearization& rhs )
{
    return lhs.region != Region::Invalid
        && rhs.region != Region::Invalid
        && lhs.slope == rhs.slope
        && lhs.intercept == rhs.intercept;
}

bool OpAmp::sameLinearization( const Linearization& lhs,
                               const Linearization& rhs )
{
    return lhs.region != Region::Invalid
        && lhs.region == rhs.region
        && sameEquation( lhs, rhs );
}

int OpAmp::regionPairIndex( Region first, Region second )
{
    int low = std::min( static_cast<int>(first), static_cast<int>(second) )+1;
    int high = std::max( static_cast<int>(first), static_cast<int>(second) )+1;
    if( low < 0 || high > 2 || low == high ) return -1;
    if( low == 0 ) return high-1;
    return 2;
}

OpAmp::Region OpAmp::remainingRegion( Region first, Region second )
{
    return static_cast<Region>(-(static_cast<int>(first)
                                +static_cast<int>(second)));
}

void OpAmp::resetSolverState()
{
    m_solverState = SolverState{};
    m_solverState.effectiveGain = std::copysign(
        std::min(std::fabs(m_gain), 1.0), m_gain );
}

void OpAmp::beginSolveEpoch( uint64_t solveEpoch )
{
    ActiveSetState& activeSet = m_solverState.activeSet;
    if( activeSet.solveEpoch == solveEpoch ) return;
    activeSet = ActiveSetState{};
    activeSet.solveEpoch = solveEpoch;
}

void OpAmp::recordStampedModel( const Linearization& model )
{
    ActiveSetState& activeSet = m_solverState.activeSet;
    activeSet.recentModels[0] = activeSet.recentModels[1];
    activeSet.recentModels[1] = model;
}

void OpAmp::initialize()
{
    added = false;
    resetSolverState();
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

    Linearization model;
    double out = 0;
    bool activeSetPivot = false;
    bool continuationJump = false;
    auto setRegionModel = [&]( Region selectedRegion )
    {
        model.region = selectedRegion;
        if( selectedRegion == Region::Negative )
        {
            model.slope = 0;
            out = m_voltNeg;
        }
        else if( selectedRegion == Region::Positive )
        {
            model.slope = 0;
            out = m_voltPos;
        }
        else
        {
            model.slope = m_solverState.effectiveGain;
            out = vd*m_solverState.effectiveGain;
        }
        model.intercept = out-model.slope*vd;
    };
    auto updateModel = [&]()
    {
        const double target = vd*m_solverState.effectiveGain;
        if     ( target > m_voltPos ) setRegionModel( Region::Positive );
        else if( target < m_voltNeg ) setRegionModel( Region::Negative );
        else                          setRegionModel( Region::Linear );
    };
    updateModel();

    // 正反馈比较器在新的瞬态时刻可能从饱和支路跨入线性区，
    // 满增益牛顿步会在两个工作区之间反复跳变。仅当新仿真时刻
    // 引起工作区变化时重启增益续接；同一时刻续接过程自身的换区必须放行。
    Simulator* simulator = Simulator::self();
    const uint64_t modelTime = simulator->circTime();
    beginSolveEpoch( simulator->solveEpoch() );
    if( modelTime != m_solverState.lastStampTime
     && m_solverState.stampedModel.region != Region::Invalid
     && model.region != m_solverState.stampedModel.region
     && std::fabs(m_solverState.effectiveGain) >= std::fabs(m_gain) )
    {
        m_solverState.effectiveGain = std::copysign(
            std::min(std::fabs(m_gain), 1.0), m_gain );
        updateModel();
    }

    // 只把同一求解轮次中完整仿射模型重复出现认作 A→B→A，
    // 避免动态电源轨或增益变化时仅凭区域编号误判循环。
    ActiveSetState& activeSet = m_solverState.activeSet;
    const Linearization& previousModel = activeSet.recentModels[0];
    const Linearization& lastModel = activeSet.recentModels[1];
    if( sameLinearization(previousModel, model)
     && !sameLinearization(lastModel, model)
     && lastModel.region != Region::Invalid )
    {
        // 先结束增益续接，再重新积累满增益历史；换基和增益跳转不在
        // 同一次迭代中混用，便于分别验证两种收敛策略。
        if( std::fabs(m_solverState.effectiveGain) < std::fabs(m_gain) )
        {
            const uint64_t solveEpoch = activeSet.solveEpoch;
            m_solverState.effectiveGain = m_gain;
            activeSet = ActiveSetState{};
            activeSet.solveEpoch = solveEpoch;
            updateModel();
            continuationJump = true;
        }
        else
        {
            const int pairIndex = regionPairIndex( model.region, lastModel.region );
            if( pairIndex >= 0 )
            {
                bool& attempted = activeSet.attemptedRegionPairs[pairIndex];
                if( !attempted )
                {
                    attempted = true;
                    setRegionModel( remainingRegion(model.region, lastModel.region) );
                    activeSetPivot = true;
                }
            }
        }
    }

    // 增益续接为连续求解提供邻近工作点。若饱和区产生相同矩阵印章，
    // 可直接跳过这些续接值，直到模型变化或达到设定增益。
    while( sameEquation(model, m_solverState.stampedModel)
        && std::fabs(m_solverState.effectiveGain) < std::fabs(m_gain) )
    {
        const double nextMagnitude = std::min( std::fabs(m_gain),
            std::fabs(m_solverState.effectiveGain)*1.5 );
        m_solverState.effectiveGain = std::copysign( nextMagnitude, m_gain );
        updateModel();
    }

    // 分段线性牛顿形式：Vout = slope*(V+ - V-) + intercept。
    // 斜率和截距未变化时，当前矩阵已精确表示该工作区。
    if( sameEquation(model, m_solverState.stampedModel) ) return;

    if( simulator->convergenceDiagnosticsEnabled() )
    {
        std::ostringstream convergenceState;
        convergenceState << "vd=" << vd
                         << " targetOut=" << out
                         << " region=" << static_cast<int>(model.region)
                         << " previousRegion="
                         << static_cast<int>(m_solverState.stampedModel.region)
                         << " slope=" << model.slope
                         << " previousSlope=" << m_solverState.stampedModel.slope
                         << " activeSetPivot=" << activeSetPivot
                         << " continuationJump=" << continuationJump
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
        m_output->stampSingAdm( positiveNode,  outputAdmitance*model.slope );
        m_output->stampSingAdm( negativeNode, -outputAdmitance*model.slope );
    }
    m_output->stampCurrent( outputAdmitance*model.intercept );

    m_solverState.stampedModel = model;
    m_solverState.lastStampTime = modelTime;
    recordStampedModel( model );
}

void OpAmp::setGain( double gain )
{
    m_gain = gain;
    resetSolverState();
    m_changed = true;
}

void OpAmp::setVoltPos( double voltage )
{
    m_voltPosDef = voltage;
    resetSolverState();
    m_changed = true;
}

void OpAmp::setVoltNeg( double voltage )
{
    m_voltNegDef = voltage;
    resetSolverState();
    m_changed = true;
}

void OpAmp::setOutImp( double imp )
{
    if( imp < cero_doub ) imp = cero_doub;
    m_outImp = imp;
    m_solverState.stampedModel = Linearization{};
    m_solverState.activeSet = ActiveSetState{};
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
    resetSolverState();
    m_changed = true;
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
