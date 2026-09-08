/***************************************************************************
 *   Copyright (C) 2016 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2025 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */

#include "e-mosfet.h"
#include "simulator.h"
#include "e-pin.h"
#include "e-node.h"
#include "mosfet_channel.h"
#include <sstream>

eMosfet::eMosfet( std::string id )
       : eResistor( id )
{
    m_Pchannel  = false;
    m_depletion = false;
    
    m_gateV     = 0;
    m_RDSon     = 1;
    m_threshold = 3;

    m_ePin.resize(3);
}
eMosfet::~eMosfet(){}

void eMosfet::stamp()
{
    m_admit = 1/m_RDSon;

    m_step = 0;
    m_gateV = 0;
    m_gm = 0;
    m_equivalentCurrent = 0;
    m_linearizationValid = false;
    m_linearizedVgs = 0;
    m_linearizedVds = 0;

    updateValues();

    if( (m_ePin[0]->isConnected())
      &&(m_ePin[1]->isConnected()) )
    {
        eResistor::stamp();

        m_ePin[0]->createCurrent();
        m_ePin[1]->createCurrent();

        m_ePin[0]->getEnode()->addToNoLinList(this);
        m_ePin[1]->getEnode()->addToNoLinList(this);

        // 跨导是漏、源矩阵行对栅、源电压的引用；提前登记以参与矩阵分组。
        if( m_ePin[2]->isConnected()
         && m_ePin[2]->getEnode() != m_ePin[1]->getEnode() )
        {
            const int gateNode = m_ePin[2]->getEnode()->getNodeNumber();
            const int sourceNode = m_ePin[1]->getEnode()->getNodeNumber();
            for( int row=0; row<2; ++row )
            {
                m_ePin[row]->addSingAdm( gateNode, 0 );
                m_ePin[row]->addSingAdm( sourceNode, 0 );
            }
        }
    }
    if( m_ePin[2]->isConnected() ) m_ePin[2]->getEnode()->addToNoLinList(this);
}

void eMosfet::voltChanged()
{
    if( !m_ePin[0]->isConnected() || !m_ePin[1]->isConnected() ) return;
    const double vs = m_ePin[1]->getVoltage();
    const double vds = m_ePin[0]->getVoltage()-vs;
    const double vgs = m_ePin[2]->isConnected() ? m_ePin[2]->getVoltage()-vs : 0;
    const double polarity = m_Pchannel ? -1 : 1;
    MosfetChannel channel = evaluateMosfetChannel(
        polarity*vds, polarity*vgs, m_Gth, m_RDSon, m_kRDSon, cero_doub );
    double current = polarity*channel.current;
    const bool controlled = m_ePin[2]->isConnected()
                         && m_ePin[2]->getEnode() != m_ePin[1]->getEnode();
    double gm = controlled ? channel.gm : 0;
    const double predicted = m_admit*vds+m_gm*vgs+m_equivalentCurrent;
    const double residual = current-predicted;
    m_gateV = std::max( polarity*vgs-m_Gth, 0.0 );

    // 检查真实沟道电流与上一轮切线的误差，不再比较补偿电流本身。
    if( m_linearizationValid && std::fabs(residual) <= 1e-9+1e-6*std::fabs(current) )
        return;

    // 限制数值线性化点的移动，避免高增益反馈把一次牛顿步拉过整个沟道区。
    // 两个端电压采用同一比例，保持栅漏短接等约束；实际节点电压不被修改。
    double fraction = 1;
    const double deltaGs = vgs-m_linearizedVgs;
    const double deltaDs = vds-m_linearizedVds;
    if( m_linearizationValid )
    {
        if( std::fabs(deltaGs) > 0.5 ) fraction = std::min(fraction, 0.5/std::fabs(deltaGs));
        if( std::fabs(deltaDs) > 2.0 ) fraction = std::min(fraction, 2.0/std::fabs(deltaDs));
    }
    double linearVgs = m_linearizedVgs+fraction*deltaGs;
    double linearVds = m_linearizedVds+fraction*deltaDs;
    channel = evaluateMosfetChannel(
        polarity*linearVds, polarity*linearVgs, m_Gth, m_RDSon, m_kRDSon, cero_doub );
    current = polarity*channel.current;
    gm = controlled ? channel.gm : 0;
    // 限步后若仍是原方程（截止或电阻限流区），不会触发电压回调。
    // 此时跳过无效中间点，保证残差不满足时不会停在同一印章上。
    if( m_linearizationValid && channel.gds == m_admit && gm == m_gm
     && current-channel.gds*linearVds-gm*linearVgs == m_equivalentCurrent )
    {
        linearVgs = vgs;
        linearVds = vds;
        channel = evaluateMosfetChannel(
            polarity*vds, polarity*vgs, m_Gth, m_RDSon, m_kRDSon, cero_doub );
        current = polarity*channel.current;
        gm = controlled ? channel.gm : 0;
    }

    Simulator* simulator = Simulator::self();
    if( simulator->convergenceDiagnosticsEnabled() )
    {
        std::ostringstream state;
        state << "Vgs=" << vgs << " Vds=" << vds << " Id=" << current
              << " residual=" << residual << " gm=" << gm << " gds=" << channel.gds;
        simulator->notCorverged( m_elmId, state.str() );
    }
    else simulator->notCorverged();

    eResistor::setAdmit( channel.gds );
    if( controlled )
    {
        const int gateNode = m_ePin[2]->getEnode()->getNodeNumber();
        const int sourceNode = m_ePin[1]->getEnode()->getNodeNumber();
        // stampSingAdm 的参数是矩阵系数的相反数，两行严格等量反向。
        m_ePin[0]->stampSingAdm( gateNode, -gm );
        m_ePin[0]->stampSingAdm( sourceNode, gm );
        m_ePin[1]->stampSingAdm( gateNode, gm );
        m_ePin[1]->stampSingAdm( sourceNode, -gm );
    }
    m_gm = gm;
    m_equivalentCurrent = current-channel.gds*linearVds-gm*linearVgs;
    m_ePin[0]->stampCurrent( -m_equivalentCurrent );
    m_ePin[1]->stampCurrent( m_equivalentCurrent );
    m_linearizationValid = true;
    m_linearizedVgs = linearVgs;
    m_linearizedVds = linearVds;
}

void eMosfet::setRDSon( double rdson )
{
    if( rdson < cero_doub ) rdson = cero_doub;
    if( rdson > 1000 ) rdson = 1000;
    m_RDSon = rdson;
    m_changed = true;
}

void eMosfet::updateVI()
{
    m_current = 0;
    if( !m_ePin[0]->isConnected() || !m_ePin[1]->isConnected() ) return;
    const double vs = m_ePin[1]->getVoltage();
    const double vds = m_ePin[0]->getVoltage()-vs;
    const double vgs = m_ePin[2]->isConnected() ? m_ePin[2]->getVoltage()-vs : 0;
    const double polarity = m_Pchannel ? -1 : 1;
    // 对外报告真实沟道电流，不能把牛顿切线电导当作器件直流电导。
    m_current = polarity*evaluateMosfetChannel(
        polarity*vds, polarity*vgs, m_Gth, m_RDSon, m_kRDSon, cero_doub ).current;
}

void eMosfet::setThreshold( double th )
{
    if( th < 0.01 ) return;
    m_threshold = th; 
    m_kRDSon = m_RDSon*(10-m_threshold);
    m_Gth = m_threshold-m_threshold/4;
    m_changed = true;
}

void eMosfet::updateValues()
{
    m_kRDSon = m_RDSon*(10-m_threshold);
    m_Gth    = m_depletion ? -m_threshold-m_threshold/4 : m_threshold-m_threshold/4;
}
