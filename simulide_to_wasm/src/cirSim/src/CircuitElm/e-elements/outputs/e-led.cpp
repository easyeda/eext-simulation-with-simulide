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

eLed::eLed( std::string id )
    : eResistor( id )
{
    m_imped = 0.6;
    m_threshold  = 1.8;
    m_maxCurrent = 0.03;
    m_vt = 0.025865;
    m_vzCoef = 1/m_vt;
    m_satCur = 1e-10;
    m_emCoef = 10;
    m_bkDown = 0;

 updateDiodeValues();
    eLed::initialize();
}
eLed::~eLed() {}

void eLed::initialize()
{
    m_converged = true;
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


    if( m_changed ) m_changed = false;
    else if( std::abs(voltPN-m_voltPN) < 1e-4
    && std::abs(m_current - m_lastCurrent)<1e-6)
    {
        m_step = 0;
        m_converged = true;
        return;
    }

    m_converged = false;

    Simulator::self()->notCorverged();

    m_step += .01;
    double gmin = m_bAdmit*std::exp( m_step );
    if( gmin > .1 ) gmin = .1;

    if( voltPN > m_vCriti && std::fabs(voltPN - m_voltPN) > m_vScale*2 ) // check new voltage; has current changed by factor of e^2?
    {
        voltPN = limitStep( voltPN, m_voltPN, m_vScale, m_vCriti );
    }
    m_voltPN = voltPN;

    double eval = std::exp( voltPN*m_vdCoef );

    if( m_bkDown == 0 || voltPN >= 0  )  // No breakdown Diode or Forward biased Zener
    {
        m_admit   = m_satCur * m_vdCoef*eval + gmin;
        m_current = m_satCur * (eval-1);
    }
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
    m_bAdmit = m_satCur * 1e-2;
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
    // m_threshold = threshold;
    m_satCur =  m_vScale/(std::exp(threshold/m_vScale)*std::sqrt(2));
    updateDiodeValues();
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
    voltChanged();

    Simulator::self()->resumeSim();
}
std::string eLed::getElementStatusData()
{
    return std::to_string(m_intensity);
}
