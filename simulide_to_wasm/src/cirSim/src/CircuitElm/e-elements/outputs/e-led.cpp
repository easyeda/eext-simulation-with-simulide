/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2025 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */

#include "e-led.h"
#include "e-pin.h"
#include "e-node.h"
#include "simulator.h"
#include <sstream>

eLed::eLed( std::string id )
    : eResistor( id )
{
    m_imped = 0.6;
    m_threshold  = 1.8;
    m_maxCurrent = 0.03;
    m_vt = 0.025865;
    m_vzCoef = 1/m_vt;
    m_satCur = 1e-10;
    m_emCoef = 2;
    m_bkDown = 0;

    updateDiodeValues();
    updtSatCur();
    eLed::initialize();
}
eLed::~eLed() {}

void eLed::initialize()
{
    m_converged = true;
    m_step       = 0;    
    m_prevStep     = 0;
    m_avgCurrent   = 0.;
    m_intensity    = 0;
    m_brightness   = 0;
    m_totalCurrent = 0;
    m_lastPeriod   = 0;


    m_admit   = m_bAdmit;
    m_voltPN  = 0;
    m_current = 0;
    m_lastCurrent = 0;
    m_lastThCurrent = 0;
}

void eLed::stamp()
{
    eResistor::stamp();

    eNode* node = m_ePin[0]->getEnode();
    if( node ) node->addToNoLinList( this );

    node = m_ePin[1]->getEnode();
    if( node ) node->addToNoLinList( this );

    m_ePin[0]->createCurrent();
    m_ePin[1]->createCurrent();
}

void eLed::voltChanged()
{
    double v0 = m_ePin[0]->getVoltage();
    double v1 = m_ePin[1]->getVoltage();
    // std::cout << "this calculate bear fuit is" << "the zero is "  << v0 << " the first is " << v1 << std::endl;
    double voltPN =  v0 - v1;

    // 端电压钳位: 反偏 LED 只有 gmin 电导, 若某个节点被推到 GV 级,
    // 由 m_admit*voltPN 构成的等效电流源会把这个异常状态锁成自洽解。
    // 钳到 ±50V 后, 反偏电流回到 nA 量级, 大电压状态不再稳定。
    const double vpnMax = 50.0;
    if( !std::isfinite(voltPN) ) voltPN = 0;
    else if( voltPN >  vpnMax )  voltPN =  vpnMax;
    else if( voltPN < -vpnMax )  voltPN = -vpnMax;

    if( m_changed ) m_changed = false;
    else if( std::abs(voltPN-m_voltPN) < 1e-2 )
    {
        m_step = 0;
        m_converged = true;
        return;
    }

    m_converged = false;

    Simulator* simulator = Simulator::self();
    if( simulator->convergenceDiagnosticsEnabled() )
    {
        std::ostringstream state;
        state << "Vpn=" << voltPN << " I=" << m_current
              << " admit=" << m_admit << " gminStep=" << m_step;
        simulator->notCorverged( m_elmId, state.str() );
    }
    else simulator->notCorverged();

    m_step += .01;
    double gmin = m_bAdmit*std::exp( m_step );
    if( gmin > .1 ) gmin = .1;

    if( voltPN > m_vCriti && std::fabs(voltPN - m_voltPN) > m_vScale*2 ) // check new voltage; has current changed by factor of e^2?
    {
        voltPN = limitStep( voltPN, m_voltPN, m_vScale, m_vCriti );
    }
    m_voltPN = voltPN;

    double vJ = voltPN - m_imped*m_current;      // 初值: 上一轮电流对应的结电压
    if( vJ > voltPN ) vJ = voltPN;               // 正向时结电压不高于端电压
    // 结电压超过阈值 1V 后电流已由串联电阻主导, 再高只会让 exp 溢出成 inf/NaN
    const double vJmax = m_threshold + 1.0;
    if( vJ > vJmax ) vJ = vJmax;
    for( int i=0; i<50; ++i )
    {
        double eJ   = std::exp( vJ*m_vdCoef );
        double f    = vJ + m_satCur*(eJ-1)*m_imped - voltPN;
        double df   = 1 + m_imped*m_satCur*eJ*m_vdCoef;
        double step = f/df;
        vJ -= step;
        if( vJ > voltPN + m_satCur*m_imped ) vJ = voltPN + m_satCur*m_imped;
        if( vJ > vJmax ) vJ = vJmax;
        if( std::fabs(step) < 1e-12 ) break;
    }
    // 兜底: 牛顿迭代产生 NaN 时回退到额定点(正向取阈值、反偏取端电压),
    // 避免用 vJmax 算出安培级电流再污染矩阵。
    if( !std::isfinite(vJ) ) vJ = std::min( voltPN, m_threshold );
    double eJ  = std::exp( vJ*m_vdCoef );
    double diJ = m_satCur*eJ*m_vdCoef;           // dI/dV结

    // LED 没有击穿模型(m_bkDown 恒为 0), 直接给出正向/反偏的统一表达式
    m_admit   = diJ/(1 + m_imped*diJ) + gmin;
    m_current = m_satCur * (eJ-1);
    eResistor::stampAdmit();

    double stCurr = m_current - m_admit*voltPN;
    updateVI();
    m_ePin[0]->stampCurrent(-stCurr );
    m_ePin[1]->stampCurrent( stCurr );


    // double ThCurrent = m_current = 0;
    // double admit = 1e-9;

    // double deltaV = m_voltPN-m_threshold;
    // if( deltaV > -1e-12 )   // Conducing
    // {
    //     admit = 1/m_imped;
    //     ThCurrent = m_threshold*admit;
    //     if( deltaV > 0 ) m_current = deltaV*admit;
    // }
    // if( admit != m_admit ) eResistor::setAdmit( admit );

    // if( ThCurrent == m_lastThCurrent ) {
    //     updateVI();
    //     m_converged = true;
    //     return;
    // }
    // m_lastThCurrent = ThCurrent;
    // m_converged = false;

    // Simulator::self()->notCorverged();

    // m_ePin[0]->stampCurrent( ThCurrent );
    // m_ePin[1]->stampCurrent(-ThCurrent );
}

void eLed::updateVI()
{
    const uint64_t step = Simulator::self()->circTime();
    uint64_t period = (step-m_prevStep);
    m_prevStep = step;
    m_lastPeriod += period;

    if( m_lastCurrent > 0 ) m_totalCurrent += m_lastCurrent*period;
    m_lastCurrent = m_current;
}

void eLed::updateDiodeValues()
{
    // 根据阈值电压计算饱和电流（使阈值电压对应m_threshold）
    m_vScale = m_emCoef * m_vt;
    m_vdCoef = 1/m_vScale;
    m_vCriti = m_vScale * std::log( m_vScale/(std::sqrt(2)*m_satCur) );
    m_zOfset = m_bkDown - m_vt*std::log(-(1-0.005/m_satCur) );
    m_vzCrit = m_vt*std::log( m_vt/(std::sqrt(2)*m_satCur) );
    m_bAdmit = std::max( m_satCur*1e-2, 1e-6 );
    m_changed = true;
    
}



inline double eLed::limitStep( double vnew, double vold, double scale, double vc )
{
    if( vold > 0 )
    {
        double arg = 1 + (vnew-vold)/scale;
        if( arg > 0 ) vnew = vold + scale*std::log( arg );
        else          vnew = vc;
    }
    else
        vnew = scale*std::log( vnew/scale );

    return vnew;
}

void eLed::setThreshold( double threshold )
{
    if( threshold < 0.01 ) return;
    m_threshold = threshold;  
    updtSatCur();
}



void eLed::updateBright()
{
    if( !Simulator::self()->isRunning() )
    {
        m_totalCurrent = 0;
        m_lastPeriod   = 0;
        m_intensity    = 0;
        return;
    }
    updateVI();

    if( Simulator::self()->isPauseDebug() )
    {
        double bright = std::pow( m_current/m_maxCurrent, 1.0/2.0 );
        m_intensity  = uint32_t(bright*255);
    }
    else{
        uint64_t psPF = Simulator::self()->realPsPF();//stepsPerFrame();
        //uint64_t sPS = Simulator::self()->stepSize();

        if( m_lastPeriod > psPF/2 ) // Update 2 times per frame
        {
            m_avgCurrent = m_totalCurrent/m_lastPeriod;
            m_brightness = std::pow( m_avgCurrent/m_maxCurrent, 1.0/2.0 );

            m_totalCurrent  = 0;
            m_lastPeriod = 0;
            m_intensity  = uint32_t(m_brightness*255);
        }
    }
}

void eLed::setRes( double resist )
{
    Simulator::self()->pauseSim();

    if( resist == 0 ) resist = 0.1;
    m_imped = resist;
    updtSatCur();     // Rs 参与标定, 保持 Vf(MaxCurrent) 语义不变
    voltChanged();

    Simulator::self()->resumeSim();
}


void eLed::updtSatCur()
{
    double vJ = m_threshold - m_imped*m_maxCurrent;   
    if( vJ > m_vScale*700 ) vJ = m_vScale*700;        // 防exp溢出(阈值异常大时)
    if( vJ < m_vScale     ) vJ = m_vScale;            // 阈值过小/串阻过大时退化为准线性
    m_satCur = m_maxCurrent/(std::exp( vJ/m_vScale )-1);
    updateDiodeValues();
}
std::string eLed::getElementStatusData()
{
    return std::to_string(m_intensity);
}
