/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

/*   Modified (C) 2025 by EasyEDA & JLC Technology Group                      *
 *   chensiyu@sz-jlc.com                                                   *
 *                                                                         */

#include "circmatrix.h"
#include "simulator.h"

// 大块稀疏 LU: SuiteSparse KLU (external/suitesparse)
#include "klu.h"

CircMatrix* CircMatrix::m_pSelf = 0l;

CircMatrix::CircMatrix()
{
    m_pSelf = this;
    m_numEnodes = 0;
}


CircMatrix::~CircMatrix()
{
    // 释放 KLU 符号/数值因子
    klu_common c; klu_defaults( &c );
    for( KLUBlock& kb : m_klu )
    {
        if( kb.num ) klu_free_numeric( (klu_numeric**)&kb.num, &c );
        if( kb.sym ) klu_free_symbolic( (klu_symbolic**)&kb.sym, &c );
    }
}

void CircMatrix::createMatrix( std::vector<eNode*> &eNodeList )
{
    // 若上一次仿真残留 KLU 因子, 先释放
    klu_common c; klu_defaults( &c );
    for( KLUBlock& kb : m_klu )
    {
        if( kb.num ) klu_free_numeric( (klu_numeric**)&kb.num, &c );
        if( kb.sym ) klu_free_symbolic( (klu_symbolic**)&kb.sym, &c );
    }
    m_klu.clear();

    m_eNodeList = &eNodeList;
    m_numEnodes = (int)eNodeList.size();

    m_circMatrix.clear();
    m_coefVect.clear();

    m_circMatrix.resize( m_numEnodes , std::vector<double>( m_numEnodes , 0.0 ) );
    m_coefVect.resize( m_numEnodes , 0.0 );

    std::cout <<"\nInitializing Matrix: "<< m_numEnodes << " eNodes\n";
    analyze();
}

void CircMatrix::analyze()
{
    std::cout << "entering analyze" << std::endl;

    // m_nodeList 表示矩阵中的非零耦合项。即使某个器件只从一侧登记
    // A 行到 B 列的项，A、B 也必须属于同一个求解块，因此这里按无向图分组。
    std::vector<std::vector<int>> adjacency( m_numEnodes+1 );
    for( int nodeNum=1; nodeNum<=m_numEnodes; ++nodeNum )
    {
        const std::vector<int>& connections = (*m_eNodeList)[nodeNum-1]->getConnections();
        for( int connectedNode : connections )
        {
            if( connectedNode == 0 ) continue; // 地节点是已知电位，不参加矩阵分组
            if( connectedNode < 1 || connectedNode > m_numEnodes )
            {
                std::cerr << "CircMatrix: invalid node connection "
                          << nodeNum << " -> " << connectedNode << std::endl;
                continue;
            }
            adjacency[nodeNum].push_back( connectedNode );
            adjacency[connectedNode].push_back( nodeNum );
        }
    }
    std::vector<bool> visited( m_numEnodes+1, false );

    m_aList.clear();
    m_aFaList.clear();
    m_bList.clear();
    m_eNodeActList.clear();
    m_permutations.clear();
    int group = 0;
    int singleNode = 0;

    for( int root=1; root<=m_numEnodes; ++root )
    {
        if( visited[root] ) continue;

        std::vector<int> nodeGroup;
        std::vector<int> pending{ root };
        visited[root] = true;
        while( !pending.empty() )
        {
            const int nodeNum = pending.back();
            pending.pop_back();
            nodeGroup.push_back( nodeNum );
            (*m_eNodeList)[nodeNum-1]->setSingle( false );

            for( int connectedNode : adjacency[nodeNum] )
            {
                if( visited[connectedNode] ) continue;
                visited[connectedNode] = true;
                pending.push_back( connectedNode );
            }
        }

        int numEnodes = (int)nodeGroup.size();
        if( numEnodes==1 )
        {
            eNode* enod = m_eNodeList->at( nodeGroup[0]-1 );
            enod->setSingle( true );
            singleNode++;
        }
        else
        {
            dp_matrix_t a;
            DenseMat ap; // temp container not used directly for pointers
            dp_vector_t b;
            std::vector<eNode*> eNodeActive;

            a.resize( numEnodes , dp_vector_t( numEnodes , nullptr ) );
            b.resize( numEnodes , nullptr );

            int ny=0;
            for( int y=0; y<m_numEnodes; ++y )
            {
                if(std::find(nodeGroup.begin(), nodeGroup.end(), y + 1) == nodeGroup.end()) 
                    continue;
                int nx=0;
                for( int x=0; x<m_numEnodes; ++x )
                {
                    if(std::find(nodeGroup.begin(), nodeGroup.end(), x + 1) == nodeGroup.end()) 
                        continue;
                    a[nx][ny] = &(m_circMatrix[x][y]);
                    nx++;
                }
                b[ny] = &(m_coefVect[y]);
                eNode* node = m_eNodeList->at(y);
                node->setNodeGroup( group );
                eNodeActive.push_back( node );
                ny++;
            }

            m_aList.push_back( std::move(a) );
            m_aFaList.emplace_back( numEnodes ); // reserve a flattened matrix of size n*n
            m_bList.push_back( std::move(b) );
            m_eNodeActList.push_back( std::move(eNodeActive) );
            m_permutations.emplace_back();

            KLUBlock kb;
            kb.n = numEnodes >= KLU_MIN_SIZE ? numEnodes : 0;  // 0 表示走稠密
            m_klu.push_back( std::move(kb) );
            group++;
        }
    }

    m_admitChanged.resize( group, true );
    m_currChanged.resize(  group, true );

    std::cout <<"CircMatrix::solveMatrix "<<group<<" Circuits\n";
    std::cout <<"CircMatrix::solveMatrix "<<singleNode<<" Single Nodes\n";
}

bool CircMatrix::solveMatrix()
{
    bool ok = true;
    for( int i=0; i<m_bList.size(); ++i )
    {
        if( !m_admitChanged[i] && !m_currChanged[i] ) continue;

        m_eNodeActive = &(m_eNodeActList[i]);
        int n = (int)m_eNodeActive->size();

        if( m_admitChanged[i] ) factorMatrix( n, i );
        if( !luSolve( n, i ) ) ok = false;

        m_currChanged[i]  = false;
        m_admitChanged[i] = false;
    }
    return ok;
}

void CircMatrix::factorMatrix( int n, int group ) // Factor matrix into Lower/Upper triangular
{
    // 大块走 KLU 稀疏 LU: 每帧导纳变化都需要重分解, 稀疏远快于稠密
    if( group < (int)m_klu.size() && m_klu[group].n == n && n >= KLU_MIN_SIZE )
    {
        buildCSC( group );
        kluFactor( group );
        // KLU 按默认 halt_if_singular 判奇异时返回空因子; 此处退回稠密 LU,
        // 与未接入 KLU 时的数值行为一致(稠密 LU 对近奇异主元只跳过不求值)。
        if( m_klu[group].kluOk ) return;
    }

    // 小块保持稠密 LU(开销低, 数值行为不变)
    dp_matrix_t &apPtrs = m_aList[group];
    DenseMat &LU = m_aFaList[group];
    std::vector<int> &perm = m_permutations[group];
    perm.resize(n);

    for(int row=0; row<n; ++row){
        double* dst = LU.rowPtr(row);
        for(int col=0; col<n; ++col){
            dst[col] = *(apPtrs[row][col]);
        }
        perm[row] = row;
    }

    for(int k=0; k<n; ++k){
        // Find pivot
        int piv = k;
        double maxv = fabs(LU(k,k));
        for(int i=k+1;i<n;++i){
            double v = fabs(LU(i,k));
            if(v > maxv){ maxv = v; piv = i; }
        }
        if(maxv == 0.0) {
            // singular; continue to next column (keep zeros)
            continue;
        }
        if(piv != k){
            // swap rows k and piv in LU
            for(int j=0;j<n;++j) std::swap(LU(k,j), LU(piv,j));
            std::swap(perm[k], perm[piv]);
        }
        double diag = LU(k,k);
        double *rowk = LU.rowPtr(k);
        for(int i=k+1;i<n;++i){
            double *rowi = LU.rowPtr(i);
            double mult = rowi[k] / diag;
            rowi[k] = mult;
            for(int j=k+1;j<n;++j) rowi[j] -= mult * rowk[j];
        }
    }
}

bool CircMatrix::luSolve( int n, int group ) // Solves the system to get voltages for each node
{
    // 大块用 KLU(KLU 分解失败走下面的稠密路径)
    if( group < (int)m_klu.size() && m_klu[group].n == n && n >= KLU_MIN_SIZE
     && m_klu[group].kluOk )
    {
        return kluSolveBlock( group );
    }

    const DenseMat &LU = m_aFaList[group];
    const dp_vector_t &bp = m_bList[group];
    const std::vector<int> &perm = m_permutations[group];

    std::vector<double> b(n);
    for(int i=0;i<n;++i) b[i] = *(bp[i]);

    std::vector<double> bperm(n);
    for(int i=0;i<n;++i) bperm[i] = b[perm[i]];

    // Forward substitution Ly = bperm (L has unit diagonal, L stored in lower part of LU)
    for(int i=0;i<n;++i){
        double sum = bperm[i];
        const double *rowi = LU.rowPtr(i);
        for(int j=0;j<i;++j) sum -= rowi[j] * bperm[j];
        bperm[i] = sum;
    }

    bool isOk = true;


    // Back substitution Ux = y
    std::vector<double> x(n);
    for(int i=n-1;i>=0;--i){
        const double *rowi = LU.rowPtr(i);
        double sum = bperm[i];
        for(int j=i+1;j<n;++j) sum -= rowi[j] * x[j];
        double div = rowi[i];
        if(div == 0.0 || !std::isfinite(div) || !std::isfinite(sum)) {
            isOk = false;
        } else{
            x[i] = sum / div;
            if( !std::isfinite(x[i]) ) isOk = false;
        }
    }

    // Write back voltages to nodes
    for(int i=0;i<n;++i){
        // std:: cout << 'the node of ' << i << 'volt is' << x[i] << std::endl;
        m_eNodeActive->at(i)->setVolt( x[i] );
    }
    return isOk;
}

// 从稠密 m_circMatrix 子块提取 CSC (Ap/Ai/Ax) 供 KLU 使用
// 注: 大块在引擎里仍是按 (row,col) 密集存取, 此处转成 CSC 送给 KLU。
void CircMatrix::buildCSC( int group )
{
    KLUBlock& kb = m_klu[group];
    int n = kb.n;
    dp_matrix_t &apPtrs = m_aList[group];  // apPtrs[row][col] = &A[row][col]


    static thread_local std::vector<int> colCount;
    if( (int)colCount.size() != n ) colCount.assign( n, 0 );
    else
        std::fill( colCount.begin(), colCount.end(), 0 );

    // 统计每列非零数(按值非零, 结构轻量)
    for( int col=0; col<n; ++col )
    {
        int cnt = 0;
        for( int row=0; row<n; ++row )
            if( *(apPtrs[row][col]) != 0.0 ) ++cnt;
        colCount[col] = cnt;
    }

    if( (int)kb.Ap.size() != n+1 ) kb.Ap.resize( n+1 );  // 保证 Ap 尺寸,否则越界写
    int nz = 0;
    for( int c=0; c<n; ++c ){ kb.Ap[c] = nz; nz += colCount[c]; }
    kb.Ap[n] = nz;

    // Ai/Ax 仅当结构(非零量)变化时才重分配;否则复用缓冲区只覆写数值
    if( (int)kb.Ai.size() != nz ){ kb.Ai.resize( nz ); kb.Ax.resize( nz ); }

    for( int col=0; col<n; ++col )
    {
        int base = kb.Ap[col];
        int k = 0;
        for( int row=0; row<n; ++row )
        {
            double v = *(apPtrs[row][col]);
            if( v != 0.0 )
            {
                kb.Ai[base+k] = row;
                kb.Ax[base+k] = v;
                ++k;
            }
        }
    }
}

// KLU 符号+数值分解; 符号部分仅在首次/结构变化时执行
void CircMatrix::kluFactor( int group )
{
    KLUBlock& kb = m_klu[group];
    klu_common Common; klu_defaults( &Common );
    kb.kluOk = false;   

    if( kb.num ) { klu_free_numeric( (klu_numeric**)&kb.num, &Common ); kb.num = nullptr; }

    if( kb.sym && (kb.symAp != kb.Ap || kb.symAi != kb.Ai) )
    {
        klu_free_symbolic( (klu_symbolic**)&kb.sym, &Common );
        kb.sym = nullptr;
        kb.symbolicDone = false;
    }

    if( !kb.symbolicDone || !kb.sym )
    {
        kb.sym = klu_analyze( kb.n, kb.Ap.data(), kb.Ai.data(), &Common );
        if( kb.sym )
        {
            kb.symAp = kb.Ap;   // 记录本次符号分解对应的结构
            kb.symAi = kb.Ai;
        }
        kb.symbolicDone = (kb.sym != nullptr);
    }
    if( !kb.sym ) return;

    kb.num = klu_factor( kb.Ap.data(), kb.Ai.data(), kb.Ax.data(),
                         (klu_symbolic*)kb.sym, &Common );
    kb.kluOk = (kb.num != nullptr);
}

// KLU 求解(复用已分解数因子), 结果写回节点
bool CircMatrix::kluSolveBlock( int group )
{
    KLUBlock& kb = m_klu[group];
    if( !kb.num || !kb.sym ) return false;
    int n = kb.n;

    const dp_vector_t &bp = m_bList[group];
    std::vector<double> b(n);
    for(int i=0;i<n;++i) b[i] = *(bp[i]);

    klu_common Common; klu_defaults( &Common );
    int ok = klu_solve( (klu_symbolic*)kb.sym, (klu_numeric*)kb.num, n, 1,
                        b.data(), &Common );

    // Write back voltages to nodes
    for(int i=0;i<n;++i) m_eNodeActive->at(i)->setVolt( b[i] );
    return ok != 0;
}
