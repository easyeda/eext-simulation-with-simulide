/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2025 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */

#pragma once

#include <array>
#include <cstdint>
#include <limits>

#include "component.h"
#include "e-element.h"

class LibraryItem;
class IoPin;

class OpAmp : public Component, public eElement
{
    public:
        OpAmp( std::string type, std::string id );
        ~OpAmp();
        
        // 静态方法，用于构建 OpAmp 对象
        static Component* construct( std::string type, std::string id );
        // 静态方法，用于获取库项目
        static LibraryItem* libraryItem();

        virtual void initialize() override;
        virtual void stamp() override;
        virtual void updateStep() override;
        virtual void voltChanged() override;

        // 获取和设置增益
        double gain() { return m_gain; }
        void setGain( double g );

        // 获取和设置输出阻抗
        double outImp() { return m_outImp; }
        void setOutImp( double imp );

        // 获取和设置正输入电压
        double voltPos() { return m_voltPosDef; }
        void setVoltPos( double v );

        // 获取和设置负输入电压
        double voltNeg() { return m_voltNegDef; }
        void setVoltNeg( double v );

        // 获取和设置电源引脚状态
        bool powerPins() {return m_powerPins; }
        void setPowerPins( bool set );

        // 获取和设置引脚切换状态
        bool switchPins() { return m_switchPins; }
        void setSwitchPins( bool s );


    protected:
        enum class Region : std::int8_t
        {
            Invalid  = -2,
            Negative = -1,
            Linear   = 0,
            Positive = 1
        };

        // 一次运放分段线性化后实际写入矩阵的仿射模型。
        struct Linearization
        {
            Region region = Region::Invalid;
            double slope = 0;
            double intercept = 0;
        };

        // 只在一次 solveCircuit 调用中有效的主动集历史。
        struct ActiveSetState
        {
            uint64_t solveEpoch = std::numeric_limits<uint64_t>::max();
            std::array<Linearization, 2> recentModels;
            // 三种区域对在一次求解中各只允许换基一次，保证尝试次数有界。
            std::array<bool, 3> attemptedRegionPairs{};
        };

        // 运放的数值求解状态；与器件配置和电气参数分开管理。
        struct SolverState
        {
            double effectiveGain = 0;
            Linearization stampedModel;
            uint64_t lastStampTime = 0;
            ActiveSetState activeSet;
        };

        // 更新属性方法,图形界面
        void udtProperties();
        void resetSolverState();
        void beginSolveEpoch( uint64_t solveEpoch );
        void recordStampedModel( const Linearization& model );

        static bool sameLinearization( const Linearization& lhs,
                                       const Linearization& rhs );
        static bool sameEquation( const Linearization& lhs,
                                  const Linearization& rhs );
        static int regionPairIndex( Region first, Region second );
        static Region remainingRegion( Region first, Region second );

        bool m_powerPins;   // 电源引脚状态
        bool m_switchPins;  // 引脚切换状态

        double m_gain;      // 增益
        double m_voltPos;   // 正输入电压
        double m_voltNeg;   // 负输入电压
        double m_voltPosDef;// 默认正输入电压
        double m_voltNegDef;// 默认负输入电压
        double m_outImp;    // 输出阻抗
        SolverState m_solverState;

        IoPin* m_inputP;    // 正输入引脚
        IoPin* m_inputN;    // 负输入引脚
        IoPin* m_output;    // 输出引脚
};

