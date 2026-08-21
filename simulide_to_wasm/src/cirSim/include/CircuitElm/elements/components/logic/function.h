/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2026 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */

#pragma once

#include "pch.h"
#include "iocomponent.h"
#include "e-element.h"

class LibraryItem;

// Function 表达式 AST 节点：setFunctions 时编译一次，voltChanged 时遍历求值。
// 不引入 AngelScript，只支持芯片包实际用到的语法子集
// （布尔 & | ! ^、算术 + - * /、比较 < > <= >= == !=、三元 ?:、变量 i/vi/o/vo、赋值 vo=）。
struct FuncExprNode {
    enum Kind { NONE, NUM, VAR, BINOP, UNOP, COND, ASSIGN };
    Kind kind = NONE;
    double num = 0;        // NUM: 数值
    int varType = 0;       // VAR/ASSIGN 目标: 0=i(布尔输入) 1=vi(输入电压) 2=o(布尔输出) 3=vo(输出电压)
    int varIdx = -1;       // VAR: 引脚索引；-1 表示当前正在求值的输出
    char op = 0;           // BINOP/UNOP: 运算符(& | ^ + - * / < > L G = N !)
    std::vector<FuncExprNode> kids;  // 子节点
};

// 函数器件：每个输出引脚由一条表达式定义（逗号分隔）。
// 布尔表达式 -> 输出逻辑电平；vo=expr 形式 -> 输出模拟电压。
class Function : public IoComponent, public eElement
{
    public:
        Function( std::string type, std::string id );
        ~Function();

        static Component* construct( std::string type, std::string id );
        static LibraryItem* libraryItem();

        virtual void stamp() override;
        virtual void voltChanged() override;
        virtual void runEvent() override { IoComponent::runOutputs(); }

        std::string functions() { return m_funcStr; }
        void setFunctions( std::string f );

        int numInputs() { return m_inpPin.size(); }
        int numOutputs() { return m_outPin.size(); }
        void setNumInputs( int inputs );
        void setNumOutputs( int outs );

    private:
        // 每个输出一个编译好的表达式根节点（空表达式 -> NONE）
        std::vector<FuncExprNode> m_exprs;
        std::string m_funcStr;

        double evalNode( const FuncExprNode& n, int curOut );
};
