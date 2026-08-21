/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2026 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */

// 轻量版 Function：不引入 AngelScript。
// 自带一个递归下降表达式求值器（AST），覆盖芯片包实际用到的语法子集：
//   变量 i<N>(布尔输入)/vi<N>(输入电压)/o<N>(布尔输出)/vo<N>(输出电压)
//   布尔 & | ! ^、算术 + - * /、比较 < > <= >= == !=、三元 ?:、赋值 vo=expr、数字常量、括号
// 支持的芯片：74HC40103/40102、74XX381/382、TL494、CD4009/4010、AD7224 等。

#include "function.h"
#include "itemlibrary.h"
#include "iopin.h"
#include "simulator.h"

#include "intProp.h"
#include "boolProp.h"
#include "stringProp.h"

#include <cctype>

// ============================================================================
// 表达式解析器（递归下降，生成 AST）。匿名命名空间，仅本文件可见。
// ============================================================================
namespace {

class Parser
{
    public:
        Parser( const std::string& s ) : m_s(s), m_pos(0), m_ok(true) {}

        FuncExprNode parse() { return parseExpr(); }
        bool ok() const { return m_ok; }

    private:
        const std::string& m_s;
        size_t m_pos;
        bool   m_ok;

        void skipSpace() {
            while( m_pos < m_s.size() && (m_s[m_pos]==' ' || m_s[m_pos]=='\t') ) m_pos++;
        }

        // 尝试解析变量标识符（大小写不敏感）：i / vi / o / vo + 可选数字
        // 成功：填充 vType/vIdx 并消费，返回 true；失败：不消费，返回 false。
        bool tryVar( int& vType, int& vIdx ) {
            if( m_pos >= m_s.size() ) return false;
            char c = (char)std::tolower( (unsigned char)m_s[m_pos] );
            int type = -1;
            size_t len = 0;
            if( c == 'v' ) {
                if( m_pos+1 >= m_s.size() ) return false;
                char c2 = (char)std::tolower( (unsigned char)m_s[m_pos+1] );
                if      ( c2 == 'i' ) { type = 1; len = 2; }   // vi
                else if ( c2 == 'o' ) { type = 3; len = 2; }   // vo
                else return false;
            } else if( c == 'i' ) { type = 0; len = 1; }
            else if( c == 'o' ) { type = 2; len = 1; }
            else return false;

            size_t numStart = m_pos + len;
            size_t p = numStart;
            while( p < m_s.size() && std::isdigit( (unsigned char)m_s[p] ) ) p++;
            int idx = -1;                                   // 不带数字 -> 当前输出
            if( p > numStart ) idx = std::stoi( m_s.substr( numStart, p - numStart ) );
            if( p < m_s.size() && std::isalpha( (unsigned char)m_s[p] ) ) return false;  // 后跟字母不是变量

            m_pos = p;
            vType = type;
            vIdx = idx;
            return true;
        }

        // 顶层：检测 vo = expr 赋值，否则普通三元
        FuncExprNode parseExpr() {
            size_t save = m_pos;
            int vType, vIdx;
            if( tryVar(vType, vIdx) && vType == 3 ) {        // 仅 vo 可作赋值左值
                size_t afterVar = m_pos;
                skipSpace();
                if( m_pos < m_s.size() && m_s[m_pos] == '='
                    && !( m_pos+1 < m_s.size() && m_s[m_pos+1] == '=' ) ) {   // 单 '=' (非 '==')
                    m_pos++;
                    FuncExprNode rhs = parseTrinary();
                    FuncExprNode node;
                    node.kind = FuncExprNode::ASSIGN;
                    node.varType = vType;
                    node.varIdx = vIdx;
                    node.kids.push_back( rhs );
                    return node;
                }
                m_pos = save;                               // 不是赋值，回退当普通变量
            } else {
                m_pos = save;
            }
            return parseTrinary();
        }

        FuncExprNode parseTrinary() {
            FuncExprNode cond = parseOr();
            skipSpace();
            if( m_pos < m_s.size() && m_s[m_pos] == '?' ) {
                m_pos++;
                FuncExprNode thenN = parseTrinary();
                skipSpace();
                if( m_pos < m_s.size() && m_s[m_pos] == ':' ) m_pos++;
                else m_ok = false;
                FuncExprNode elseN = parseTrinary();
                FuncExprNode n;
                n.kind = FuncExprNode::COND;
                n.kids.push_back( cond );
                n.kids.push_back( thenN );
                n.kids.push_back( elseN );
                return n;
            }
            return cond;
        }

        FuncExprNode parseOr() {
            FuncExprNode left = parseAnd();
            while( true ) {
                skipSpace();
                if( m_pos < m_s.size() && m_s[m_pos] == '|' ) {
                    m_pos++;
                    if( m_pos < m_s.size() && m_s[m_pos] == '|' ) m_pos++;   // 接受 ||
                    left = makeBin( '|', left, parseAnd() );
                } else break;
            }
            return left;
        }

        FuncExprNode parseAnd() {
            FuncExprNode left = parseXor();
            while( true ) {
                skipSpace();
                if( m_pos < m_s.size() && m_s[m_pos] == '&' ) {
                    m_pos++;
                    if( m_pos < m_s.size() && m_s[m_pos] == '&' ) m_pos++;   // 接受 &&
                    left = makeBin( '&', left, parseXor() );
                } else break;
            }
            return left;
        }

        FuncExprNode parseXor() {
            FuncExprNode left = parseCmp();
            while( true ) {
                skipSpace();
                if( m_pos < m_s.size() && m_s[m_pos] == '^' ) {
                    m_pos++;
                    if( m_pos < m_s.size() && m_s[m_pos] == '^' ) m_pos++;   // 接受 ^^
                    left = makeBin( '^', left, parseCmp() );
                } else break;
            }
            return left;
        }

        FuncExprNode parseCmp() {
            FuncExprNode left = parseAdd();
            skipSpace();
            char op = 0;
            if( m_pos+1 < m_s.size() ) {
                char c1 = m_s[m_pos], c2 = m_s[m_pos+1];
                if      ( c1=='<' && c2=='=' ) { op='L'; m_pos+=2; }   // <=
                else if ( c1=='>' && c2=='=' ) { op='G'; m_pos+=2; }   // >=
                else if ( c1=='=' && c2=='=' ) { op='E'; m_pos+=2; }   // ==
                else if ( c1=='!' && c2=='=' ) { op='N'; m_pos+=2; }   // !=
            }
            if( op==0 && m_pos < m_s.size() ) {
                if      ( m_s[m_pos]=='<' ) { op='<'; m_pos++; }
                else if ( m_s[m_pos]=='>' ) { op='>'; m_pos++; }
            }
            if( op ) return makeBin( op, left, parseAdd() );
            return left;
        }

        FuncExprNode parseAdd() {
            FuncExprNode left = parseMul();
            while( true ) {
                skipSpace();
                if( m_pos < m_s.size() && (m_s[m_pos]=='+' || m_s[m_pos]=='-') ) {
                    char op = m_s[m_pos++];
                    left = makeBin( op, left, parseMul() );
                } else break;
            }
            return left;
        }

        FuncExprNode parseMul() {
            FuncExprNode left = parseUnary();
            while( true ) {
                skipSpace();
                if( m_pos < m_s.size() && (m_s[m_pos]=='*' || m_s[m_pos]=='/') ) {
                    char op = m_s[m_pos++];
                    left = makeBin( op, left, parseUnary() );
                } else break;
            }
            return left;
        }

        FuncExprNode parseUnary() {
            skipSpace();
            if( m_pos < m_s.size() && m_s[m_pos]=='!' ) {
                m_pos++;
                FuncExprNode n; n.kind = FuncExprNode::UNOP; n.op = '!';
                n.kids.push_back( parseUnary() );
                return n;
            }
            if( m_pos < m_s.size() && m_s[m_pos]=='-' ) {
                m_pos++;
                FuncExprNode n; n.kind = FuncExprNode::UNOP; n.op = '-';
                n.kids.push_back( parseUnary() );
                return n;
            }
            return parsePrimary();
        }

        FuncExprNode parsePrimary() {
            skipSpace();
            if( m_pos >= m_s.size() ) { m_ok = false; return FuncExprNode(); }
            char c = m_s[m_pos];

            if( c == '(' ) {
                m_pos++;
                FuncExprNode n = parseTrinary();
                skipSpace();
                if( m_pos < m_s.size() && m_s[m_pos] == ')' ) m_pos++;
                else m_ok = false;
                return n;
            }
            if( std::isdigit((unsigned char)c) || c == '.' ) {        // 数字（含 .5 这种）
                size_t start = m_pos;
                while( m_pos < m_s.size() && (std::isdigit((unsigned char)m_s[m_pos]) || m_s[m_pos]=='.') ) m_pos++;
                FuncExprNode n; n.kind = FuncExprNode::NUM;
                try { n.num = std::stod( m_s.substr( start, m_pos - start ) ); }
                catch(...) { m_ok = false; }
                return n;
            }
            int vType, vIdx;
            if( tryVar(vType, vIdx) ) {
                FuncExprNode n; n.kind = FuncExprNode::VAR;
                n.varType = vType; n.varIdx = vIdx;
                return n;
            }
            m_ok = false;
            return FuncExprNode();
        }

        FuncExprNode makeBin( char op, const FuncExprNode& a, const FuncExprNode& b ) {
            FuncExprNode n; n.kind = FuncExprNode::BINOP; n.op = op;
            n.kids.push_back( a );
            n.kids.push_back( b );
            return n;
        }
};

} // namespace

// ============================================================================
// Function 器件实现
// ============================================================================

Component* Function::construct( std::string type, std::string id )
{ return new Function( type, id ); }

LibraryItem* Function::libraryItem()
{
    return new LibraryItem(
        "Function",
        "Arithmetic",
        "subc.png",
        "Function",
        Function::construct );
}

Function::Function( std::string type, std::string id )
        : IoComponent( type, id )
        , eElement( id )
{
    m_width  = 4;
    m_height = 4;

    setNumInputs( 2 );
    setNumOutputs( 1 );
    setFunctions( "i0|i1" );

    Simulator::self()->addToUpdateList( this );

    addPropGroup( { "Main", {
        new intProp   <Function>("Num_Inputs",  "Input Size",  "_Pins", this
                                , &Function::numInputs,  &Function::setNumInputs,  propNoCopy, "uint" ),
        new intProp   <Function>("Num_Outputs", "Output Size", "_Pins", this
                                , &Function::numOutputs, &Function::setNumOutputs, propNoCopy, "uint" ),
        new stringProp<Function>("Functions",   "Functions",   "",      this
                                , &Function::functions,  &Function::setFunctions ),
    }, groupNoCopy } );

    // Electric 组：合并 input/output 属性（cirSim 用 std::vector 手动拼接）
    std::vector<comProperty*> inputProps  = IoComponent::inputProps();
    std::vector<comProperty*> outputProps = IoComponent::outputProps();
    std::vector<comProperty*> outputType  = IoComponent::outputType();
    std::vector<comProperty*> electric;
    electric.reserve( inputProps.size()+outputProps.size()+outputType.size() );
    electric.insert( electric.end(), inputProps.begin(),  inputProps.end() );
    electric.insert( electric.end(), outputProps.begin(), outputProps.end() );
    electric.insert( electric.end(), outputType.begin(),  outputType.end() );
    addPropGroup( { "Electric", electric, 0 } );

    addPropGroup( { "Timing", IoComponent::edgeProps(), 0 } );
}
Function::~Function(){}

void Function::stamp()
{
    IoComponent::initState();
    for( uint32_t i=0; i<m_inpPin.size(); ++i )
        m_inpPin[i]->changeCallBack( this );
}

void Function::voltChanged()
{
    bool hasBool = false;
    uint32_t boolMask = 0;
    uint32_t n = std::min( m_exprs.size(), m_outPin.size() );

    for( uint32_t i=0; i<n; ++i )
    {
        const FuncExprNode& node = m_exprs[i];
        if( node.kind == FuncExprNode::NONE ) continue;

        double val = evalNode( node, i );

        if( node.kind == FuncExprNode::ASSIGN ) {            // 模拟输出 vo = expr
            m_outPin[i]->setOutHighV( val );
            m_outPin[i]->setOutStatFast( true );
        } else {                                            // 布尔输出
            hasBool = true;
            if( val != 0.0 ) boolMask |= (1u << i);
        }
    }
    if( hasBool ) {
        m_nextOutVal = boolMask;
        IoComponent::scheduleOutPuts( this );
    }
}

void Function::setFunctions( std::string f )
{
    m_funcStr = f;

    // 按逗号分割（括号内的逗号不分割）
    std::vector<std::string> parts;
    int depth = 0;
    std::string cur;
    for( char c : f ) {
        if( c == '(' ) depth++;
        else if( c == ')' ) depth--;
        if( c == ',' && depth == 0 ) { parts.push_back(cur); cur.clear(); }
        else cur += c;
    }
    parts.push_back(cur);

    m_exprs.clear();
    m_exprs.reserve( parts.size() );
    for( std::string s : parts )
    {
        while( !s.empty() && (s.front()==' ' || s.front()=='\t') ) s.erase(0,1);
        while( !s.empty() && (s.back() ==' ' || s.back() =='\t') ) s.pop_back();
        if( s.empty() ) { m_exprs.push_back( FuncExprNode() ); continue; }   // 空输出 -> NONE

        Parser p(s);
        FuncExprNode node = p.parse();
        if( !p.ok() ) node.kind = FuncExprNode::NONE;        // 解析失败：忽略该输出
        m_exprs.push_back( node );
    }
}

void Function::setNumInputs( int inputs )
{
    if( inputs < 1 ) return;
    setNumInps( inputs, "I" );
}
void Function::setNumOutputs( int outs )
{
    if( outs < 1 ) return;
    setNumOuts( outs, "O" );
}

double Function::evalNode( const FuncExprNode& n, int curOut )
{
    switch( n.kind )
    {
        case FuncExprNode::NONE: return 0;
        case FuncExprNode::NUM:  return n.num;
        case FuncExprNode::VAR: {
            int idx = (n.varIdx < 0) ? curOut : n.varIdx;
            if( idx < 0 ) return 0;
            switch( n.varType ) {
                case 0: return (idx < (int)m_inpPin.size() && m_inpPin[idx]->getInpState()) ? 1.0 : 0.0;
                case 1: return (idx < (int)m_inpPin.size()) ? m_inpPin[idx]->getVoltage() : 0.0;
                case 2: return (idx < (int)m_outPin.size() && m_outPin[idx]->getOutState()) ? 1.0 : 0.0;
                case 3: return (idx < (int)m_outPin.size()) ? m_outPin[idx]->getVoltage() : 0.0;
            }
            return 0;
        }
        case FuncExprNode::UNOP: {
            double v = evalNode( n.kids[0], curOut );
            if( n.op == '!' ) return v == 0.0 ? 1.0 : 0.0;
            if( n.op == '-' ) return -v;
            return 0;
        }
        case FuncExprNode::BINOP: {
            double a = evalNode( n.kids[0], curOut );
            double b = evalNode( n.kids[1], curOut );
            switch( n.op ) {
                case '&': return (a!=0.0 && b!=0.0) ? 1.0 : 0.0;
                case '|': return (a!=0.0 || b!=0.0) ? 1.0 : 0.0;
                case '^': return ((a!=0.0) != (b!=0.0)) ? 1.0 : 0.0;
                case '+': return a + b;
                case '-': return a - b;
                case '*': return a * b;
                case '/': return b!=0.0 ? a/b : 0.0;
                case '<': return a <  b ? 1.0 : 0.0;
                case '>': return a >  b ? 1.0 : 0.0;
                case 'L': return a <= b ? 1.0 : 0.0;   // <=
                case 'G': return a >= b ? 1.0 : 0.0;   // >=
                case 'E': return a == b ? 1.0 : 0.0;   // ==
                case 'N': return a != b ? 1.0 : 0.0;   // !=
            }
            return 0;
        }
        case FuncExprNode::COND: {
            double c = evalNode( n.kids[0], curOut );
            return c != 0.0 ? evalNode( n.kids[1], curOut )
                           : evalNode( n.kids[2], curOut );
        }
        case FuncExprNode::ASSIGN:
            return evalNode( n.kids[0], curOut );
    }
    return 0;
}
