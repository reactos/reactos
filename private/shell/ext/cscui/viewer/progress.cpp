#include "pch.h"
#pragma hdrstop

#include "cachview.h"
#include "progress.h"
#include "ccinline.h"


ProgressBar::~ProgressBar(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("ProgressBar::~ProgressBar")));

    m_PBSubclass.KillTimer();
    m_PBSubclass.Cancel();
    delete m_pSource;
}


void
ProgressBar::Reset(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("ProgressBar::Reset")));

    DBGASSERT((NULL != m_hwndPB));
    DBGASSERT((NULL != m_pSource));

    m_PBSubclass.KillTimer();
    ProgressBar_SetPos(m_hwndPB, 0);
    m_cMaxPos = m_pSource->GetMaxCount() - 1;
    ProgressBar_SetRange(m_hwndPB, 0, m_cMaxPos);
    m_PBSubclass.StartTimer(m_cUpdateIntervalMs);
}

void
ProgressBar::Update(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("ProgressBar::Update")));

    DBGASSERT((NULL != m_hwndPB));
    DBGASSERT((NULL != m_pSource));
    int c = 0;

    if (m_bKillPending)
    {
        m_bKillPending = false;
        m_PBSubclass.KillTimer();
        if (NULL != m_hwndNotify)
        {
            NMHDR nm;
            nm.hwndFrom = m_hwndPB;
            nm.idFrom   = GetWindowLong(m_hwndPB, GWL_ID);
            nm.code     = PBN_100PCT;
            SendMessage(m_hwndNotify, WM_NOTIFY, 0, (LPARAM)&nm);
        }
    }
    else
    {
        c = m_pSource->GetCount();
        DBGPRINT((DM_VIEW, DL_LOW, TEXT("\t%d of %d loaded"), c, m_cMaxPos));

        m_bKillPending = (c >= m_cMaxPos);
    }
    ProgressBar_SetPos(m_hwndPB, c);
}


bool
ProgressBar::CommonCreate(
    HWND hwndPB,
    HWND hwndNotify,
    int cUpdateIntervalMs
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("ProgressBar::CommonCreate")));
    DBGASSERT((NULL != hwndPB));

    m_hwndPB            = hwndPB;
    m_hwndNotify        = hwndNotify;
    m_cUpdateIntervalMs = cUpdateIntervalMs;
    delete m_pSource;
    m_PBSubclass.Cancel();
    return m_PBSubclass.Initialize(m_hwndPB);
}

bool
ProgressBar::Create(
    HWND hwndPB,
    HWND hwndNotify,
    const CacheView& view,
    int cUpdateIntervalMs,
    const CscObjTree& tree
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("ProgressBar::Create - Tree source")));
    if (CommonCreate(hwndPB, hwndNotify, cUpdateIntervalMs))
        m_pSource = new TreeSource(view, tree);

    return NULL != m_pSource;
}


bool
ProgressBar::Create(
    HWND hwndPB,
    HWND hwndNotify,
    const CacheView& view,
    int cUpdateIntervalMs,
    const CscShare& share
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("ProgressBar::Create - Share source")));
    if (CommonCreate(hwndPB, hwndNotify, cUpdateIntervalMs))
        m_pSource = new ShareSource(view, share);

    return NULL != m_pSource;

}


bool
ProgressBar::Create(
    HWND hwndPB,
    HWND hwndNotify,
    int cUpdateIntervalMs
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("ProgressBar::Create - Null source")));
     if (CommonCreate(hwndPB, hwndNotify, cUpdateIntervalMs))
        m_pSource = new NullSource();

    return NULL != m_pSource;
}


void 
ProgressBar::Subclass::StartTimer(
    int cUpdateIntervalMs
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("ProgressBar::Subclass::StartTimer")));
    ::SetTimer(GetWindow(), m_idTimer, (UINT)cUpdateIntervalMs, NULL);
    m_bTimerIsDead = false;
}

void 
ProgressBar::Subclass::KillTimer(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("ProgressBar::Subclass::KillTimer")));
    if (!m_bTimerIsDead)
        ::KillTimer(GetWindow(), m_idTimer);
    m_bTimerIsDead = true;
}


LRESULT
ProgressBar::Subclass::HandleMessages(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(message)
    {
        case WM_TIMER:
            if (wParam == m_idTimer)
            {
                DBGPRINT((DM_VIEW, DL_LOW, TEXT("Rcv'd Progress bar timer msg")));
                m_bar.Update();
            }
            break;

        case WM_DESTROY:
            KillTimer();
            break;

        default:
            break;
    }
    return 0;
}

int
ProgressBar::ViewSource::GetCount(
    void
    ) const
{
    return m_view.ConsideredObjectCount();
}


int ProgressBar::ShareSource::GetMaxCount(
    void
    ) const
{
    return m_srcView.GetMaxCount() + (m_share.GetFileCount() * 2);
}

     
int ProgressBar::ShareSource::GetCount(
    void
    ) const
{
    return m_srcView.GetCount() + m_share.GetLoadedFileCount();
}


int ProgressBar::TreeSource::GetMaxCount(
    void
    ) const
{
    return m_srcView.GetMaxCount() + (m_tree.GetObjectCount() * 2);
}


int ProgressBar::TreeSource::GetCount(
    void
    ) const
{ 
    return m_srcView.GetCount() + m_tree.GetLoadedObjectCount();
}




