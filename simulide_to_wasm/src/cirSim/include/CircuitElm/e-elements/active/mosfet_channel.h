#pragma once

#include <algorithm>

// 保留原有简化沟道电流方程，同时给出牛顿线性化所需的两个偏导数。
// 电压已按沟道极性归一化；此处不加入体二极管或新的器件参数。
struct MosfetChannel
{
    double current;
    double gds;
    double gm;
};

inline MosfetChannel evaluateMosfetChannel( double vds, double vgs,
                                            double threshold, double rdson,
                                            double scale, double offConductance )
{
    const double overdrive = vgs-threshold;
    if( overdrive <= 0 ) return { offConductance*vds, offConductance, 0 };

    const double channelV = std::min( vds, overdrive );
    const double modulation = 1+vds/100;
    const double charge = overdrive*channelV-channelV*channelV/2;
    const double current = charge*modulation/scale;
    const double ohmicLimit = vds/rdson;
    if( current > ohmicLimit ) return { ohmicLimit, 1/rdson, 0 };

    const double gm = channelV*modulation/scale;
    const double gds = ((vds < overdrive ? overdrive-vds : 0)*modulation
                       +charge/100)/scale;
    return { current, gds, gm };
}
