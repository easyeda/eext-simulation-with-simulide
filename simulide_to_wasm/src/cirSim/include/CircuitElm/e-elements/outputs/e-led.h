/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2025 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */

#pragma once

#include "pch.h"
#include "e-resistor.h"

class eLed : public eResistor
{
    public:
        eLed( std::string id );
        ~eLed();

        virtual void stamp() override;
        virtual void initialize() override;
        virtual void voltChanged() override;

        double threshold() { return m_threshold; }
        void  setThreshold( double threshold );

        double maxCurrent()             { return m_maxCurrent; }
        void  setMaxCurrent( double c ) { m_maxCurrent = c; m_changed = true; }

        virtual void   setRes( double resist ) override ;
        virtual double res() override { return m_imped; }
        virtual std::string getElementStatusData() override;


        double brightness() { return m_brightness; }
        uint32_t intensity(){ return m_intensity; }
        double overCurrent() { return m_avgCurrent/m_maxCurrent; }

        void updateBright();

    protected:
        virtual void updateVI() override;

        // LED特有属性（亮度相关）
        bool m_converged;

        uint64_t m_prevStep;
        uint32_t m_intensity;

        double m_maxCurrent;
        double m_avgCurrent;
        double m_totalCurrent;
        double m_lastCurrent;
        double m_lastPeriod;
        double m_brightness;

        double m_voltPN;
        double m_threshold;
        double m_imped;
        double m_lastThCurrent;

        double m_vt;
        double m_vzCoef;
        double m_vScale;
        double m_vdCoef;
        double m_vCriti;
        double m_zOfset;
        double m_vzCrit;
        double m_bAdmit;
        double m_satCur;
        double m_emCoef;
        double m_bkDown;
        double m_step;
        bool m_changed;

        inline double limitStep( double vnew, double vold, double scale, double vc );
        void updateDiodeValues();
};

