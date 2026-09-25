// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Debug dump per-frame info.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#if DBG
#define D3DLOG_ENABLED 1
#define IF_D3DLOG(x) if (IsTagEnabled(tagD3DLog)){x}
#else
#define D3DLOG_ENABLED 0
#define IF_D3DLOG(x)
#endif

#define D3DLOG_INC(m)   IF_D3DLOG(m_pDevice->m_log.m_pCurrentFrame->data[D3DLogFrame::m]++;)
#define D3DLOG_SET(m,v) IF_D3DLOG(m_pDevice->m_log.m_pCurrentFrame->data[D3DLogFrame::m] = v;)
#define D3DLOG_ADD(m,v) IF_D3DLOG(m_pDevice->m_log.m_pCurrentFrame->data[D3DLogFrame::m] += v;)

#if DBG

ExternTag(tagD3DLog);

#define D3DLOG_MAX_FRAMES 100

#define D3DLOG_FIELDS(m)\
    m(tanksCreated                 , "Tanks Created"                    )\
    m(tanksReused                  , "Tanks Reused"                     )\
    m(lazyTanksDestroyed           , "Lazy Tanks Destroyed"             )\
    m(stubsDestroyed               , "Stubs Destroyed"                  )\
    m(tanksDestroyedOnDestruction  , "Tanks Destroyed On Destruction"   )\
    m(tmpTanksDestroyed            , "Tmp Tanks Destroyed"              )\
    m(smallPersTanksDestroyed      , "Small Pers Tanks Destroyed"       )\
    m(smallReuseTanksDestroyed     , "Small Reuse Tanks Destroyed"      )\
    m(tanksTotal                   , "Tanks Total"                      )\
    m(subglyphsRegenerated         , "Subglyphs Regenerated"            )\
    m(persSubglyphsRegenerated     , "Persistent Subglyphs Regenerated" )\
    m(subglyphsEvicted             , "Subglyphs Reanimated"             )\
    m(subglyphsReused              , "Subglyphs Reused"                 )\
    m(pixelsRegenerated            , "Pixels Regenerated"               )\
    m(pixelsReused                 , "Pixels Reused"                    )\


struct D3DLogFrame
{
    enum {
#define D3DLOG_ENUM(t,d) t,
        D3DLOG_FIELDS(D3DLOG_ENUM)
        size
    };

    int data[size];
};

class CD3DLog
{
public:
    CD3DLog() {m_pCurrentFrame = m_data;}
    ~CD3DLog() {Dump();}
    void OnPresent() {if (m_pCurrentFrame != m_data + D3DLOG_MAX_FRAMES - 1) m_pCurrentFrame++;}
    D3DLogFrame* m_pCurrentFrame;
private:
    D3DLogFrame m_data[D3DLOG_MAX_FRAMES];
    static char const* const m_frameNames[D3DLogFrame::size];
    FILE* m_pFile;
    void Dump();
    void DumpHead();
    void DumpRow(__in_ecount(1) const D3DLogFrame* pFrame);
    void DumpTail();
};

#endif //DBG


