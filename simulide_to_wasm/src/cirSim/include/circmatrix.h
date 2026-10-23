/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2025 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */

#pragma once

#include "pch.h"
#include "e-node.h"

class CircMatrix
{
    typedef std::vector<double>      d_vector_t;  
    typedef std::vector<double*>     dp_vector_t;
    struct DenseMat {
        int n=0;
        std::vector<double> data;
        DenseMat() = default;
        DenseMat(int n_): n(n_), data(n_>0 ? (size_t)n_*n_ : 0, 0.0) {}
        void resize(int n_){ n=n_; data.assign((size_t)n*n, 0.0); }
        inline double &operator()(int r,int c){ return data[(size_t)r*n + c]; }
        inline const double &operator()(int r,int c) const{ return data[(size_t)r*n + c]; }
        inline double *rowPtr(int r){ return data.data() + (size_t)r*n; }
        inline const double *rowPtr(int r) const { return data.data() + (size_t)r*n; } 
    };
    typedef std::vector<dp_vector_t> dp_matrix_t;

public:
    CircMatrix();
    ~CircMatrix();

    static CircMatrix* self() { return m_pSelf; }

    void createMatrix( std::vector<eNode*> &eNodeList );
    bool solveMatrix();

    inline void stampDiagonal( int group, int n, double value )
    {
        m_admitChanged[group] = true;
        m_circMatrix[n-1][n-1] = value;     
    }

    inline void stampMatrix( int row, int col, double value )
    {
        m_circMatrix[row-1][col-1] = value;     
    }

    inline void stampCoef( int group, int row, double value )
    {
        m_currChanged[group] = true;
        m_coefVect[row-1] = value;
    }

private:
    static CircMatrix* m_pSelf;

    void analyze();

    // optimized implementations
    void factorMatrix( int n, int group );
    bool luSolve( int n, int group );

    // ---- KLU 稀疏求解(大块专用) ----
    // 大块(节点数>=KLU_MIN_SIZE)用 SuiteSparse KLU 稀疏 LU,
    // 小块保留稠密 LU(开销更低)。
    // CSC 结构: Ap[i..i+1) 给出列 i 的行索引段 Ai[nz]，Ax 对应数值。
    struct KLUBlock {
        int n = 0;
        std::vector<int>    Ap;     // CSC 列指针 (n+1)
        std::vector<int>    Ai;     // 行索引
        std::vector<double> Ax;     // 数值(与 Ai 各自独立, 避免扰动)
        std::vector<int>    symAp;  // 符号分解时的结构快照(用于检测稀疏模式变化)
        std::vector<int>    symAi;
        void* sym   = nullptr;      // klu_symbolic*
        void* num   = nullptr;      // klu_numeric*
        bool  symbolicDone = false;
        bool  kluOk = false;        // KLU 分解成功; 失败时退回稠密 LU
    };
    static const int KLU_MIN_SIZE = 64;   // 块尺寸达到此值才走稀疏

    std::vector<KLUBlock> m_klu;          // 与 m_aFaList 等长, 无效块为空

    void buildCSC( int group );           // 从稠密 m_circMatrix 子块提取 CSC
    void kluFactor( int group );          // 符号+数值分解(复用时跳过符号)
    bool kluSolveBlock( int group );      // 前代回代, 写回节点电压        

    int m_numEnodes;
    std::vector<eNode*>* m_eNodeList;

    std::vector<dp_matrix_t> m_aList;  
    std::vector<DenseMat>  m_aFaList;  
    std::vector<dp_vector_t> m_bList;   

    std::vector<std::vector<int>> m_permutations; 

    std::vector<bool>    m_admitChanged;
    std::vector<bool>    m_currChanged;
    std::vector<eNode*>*       m_eNodeActive;
    std::vector<std::vector<eNode*>> m_eNodeActList;

    std::vector<std::vector<double>> m_circMatrix; 
    std::vector<double> m_coefVect;
};


