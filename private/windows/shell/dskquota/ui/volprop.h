#ifndef _INC_DSKQUOTA_VOLPROP_H
#define _INC_DSKQUOTA_VOLPROP_H
///////////////////////////////////////////////////////////////////////////////
/*  File: volprop.h

    Description: Provides declarations for quota property pages.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_DSKQUOTA_PRSHTEXT_H
#   include "prshtext.h"
#endif
#ifndef _INC_DSKQUOTA_DETAILS_H
#   include "details.h"
#endif

#ifdef POLICY_MMC_SNAPIN
#   ifndef _INC_DSKQUOTA_POLICY_H
#       include "policy.h"
#   endif
#endif

#include "resource.h"


const DWORD IDT_STATUS_UPDATE              = 1;
const DWORD STATUS_UPDATE_TIMER_PERIOD     = 2000; // Update every 2 sec.

#define TLM_SETSTATE (WM_USER + 100)  // TLM = Traffic Light Message.

//
// Volume property page.
//
class VolumePropPage : public DiskQuotaPropSheetExt
{
    protected:
            class TrafficLight
            {
                private:
                    HWND m_hwndAnimateCtl;
                    INT m_idAviClipRes;

                    //
                    // Prevent copy.
                    //
                    TrafficLight(const TrafficLight& rhs);
                    TrafficLight& operator = (const TrafficLight& rhs);

                public:
                    TrafficLight(VOID)
                        : m_hwndAnimateCtl(NULL),
                          m_idAviClipRes(-1)
                          { }

                    TrafficLight(HWND hwndAnimateCtl, INT idAviClipRes)
                        : m_hwndAnimateCtl(hwndAnimateCtl),
                          m_idAviClipRes(idAviClipRes)
                    {
                        Initialize(hwndAnimateCtl, idAviClipRes);
                    }

                    VOID Initialize(HWND hwndAnimateCtl, INT idAviClipRes);

                    ~TrafficLight(VOID)
                        { Animate_Close(m_hwndAnimateCtl); }

                    enum { YELLOW, OFF, RED, GREEN, FLASHING_YELLOW };

                    VOID Show(INT eShow);

                    INT_PTR ForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
            };

        DWORD          m_dwQuotaState;
        DWORD          m_dwQuotaLogFlags;
        LONGLONG       m_llDefaultQuotaThreshold;
        LONGLONG       m_llDefaultQuotaLimit;
        UINT64         m_cVolumeMaxBytes;
        UINT_PTR       m_idStatusUpdateTimer;
        DWORD          m_dwLastStatusMsgID;
        int            m_idCtlNextFocus;
        DetailsView   *m_pDetailsView;
        XBytes        *m_pxbDefaultLimit;
        XBytes        *m_pxbDefaultThreshold;
        TrafficLight  m_TrafficLight;


        virtual INT_PTR OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);

        INT_PTR OnContextMenu(HWND hwndItem, int xPos, int yPos);
        INT_PTR OnHelp(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnTimer(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnButtonDetails(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnButtonEventLog(HWND hDlg, WPARAM wParam, LPARAM lParam);

        //
        // PSN_xxxx handlers.
        //
        virtual INT_PTR OnSheetNotifyApply(HWND hDlg, WPARAM wParam, LPARAM lParam);
        virtual INT_PTR OnSheetNotifyKillActive(HWND hDlg, WPARAM wParam, LPARAM lParam);
        virtual INT_PTR OnSheetNotifyReset(HWND hDlg, WPARAM wParam, LPARAM lParam);
        virtual INT_PTR OnSheetNotifySetActive(HWND hDlg, WPARAM wParam, LPARAM lParam);

        //
        // EN_xxxx handlers.
        //
        INT_PTR OnEditNotifyUpdate(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnEditNotifyKillFocus(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnEditNotifySetFocus(HWND hDlg, WPARAM wParam, LPARAM lParam);

        //
        // CBN_xxxx handlers.
        //
        INT_PTR OnComboNotifySelChange(HWND hDlg, WPARAM wParam, LPARAM lParam);


        HRESULT UpdateControls(HWND hDlg) const;
        HRESULT InitializeControls(HWND hDlg);
        HRESULT EnableControls(HWND hDlg);
        HRESULT RefreshCachedVolumeQuotaInfo(VOID);
        HRESULT ApplySettings(HWND hDlg);
        HRESULT QuotaStateFromControls(HWND hDlg, LPDWORD pdwState) const;
        HRESULT LogFlagsFromControls(HWND hDlg, LPDWORD pdwLogFlags) const;
        BOOL ActivateExistingDetailsView(VOID) const;
        bool SetByPolicy(LPCTSTR pszPolicyValue);

        HRESULT UpdateStatusIndicators(HWND hDlg);

        VOID SetStatusUpdateTimer(HWND hDlg)
            {
                if (0 == m_idStatusUpdateTimer)
                    m_idStatusUpdateTimer = SetTimer(hDlg,
                                                     IDT_STATUS_UPDATE,
                                                     STATUS_UPDATE_TIMER_PERIOD,
                                                     NULL);
            }
        VOID KillStatusUpdateTimer(HWND hDlg)
            {
                if (0 != m_idStatusUpdateTimer)
                {
                    KillTimer(hDlg, m_idStatusUpdateTimer);
                    m_idStatusUpdateTimer = 0;
                }
            }

        //
        // Prevent copy.
        //
        VolumePropPage(const VolumePropPage& rhs);
        VolumePropPage& operator = (const VolumePropPage& rhs);

    public:
        VolumePropPage(VOID);
        ~VolumePropPage(VOID);

        //
        // Dialog Proc callback.
        //
        static INT_PTR APIENTRY DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

#ifdef POLICY_MMC_SNAPIN

//
// A specialization of VolumePropPage to address the specific needs of
// of the disk quota policy property sheet displayed from within MMC.  The
// similarities between the two classes called for reusing as much of the
// VolumePropPage implementation as possible.
// The differences from VolumePropPage are:
//
//  1. "Quota Entries" and "Event Log" buttons are hidden.
//  2. Traffic light and status messages are hidden.
//  3. Page is not opened up "over" a specific volume.
//  4. When user selects "OK" or "Apply", quota information is written to
//     the registry.
//
class CSnapInCompData; // fwd decl.
class SnapInVolPropPage : public VolumePropPage
{
    public:
        SnapInVolPropPage(void)
            : m_pPolicy(NULL) { }

        ~SnapInVolPropPage(void);

    private:
        LPDISKQUOTAPOLICY m_pPolicy;

        virtual INT_PTR OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
        virtual HRESULT CreateDiskQuotaPolicyObject(IDiskQuotaPolicy **ppOut);

        //
        // PSN_xxxx handlers.
        //
        virtual INT_PTR OnSheetNotifyApply(HWND hDlg, WPARAM wParam, LPARAM lParam);
        virtual INT_PTR OnSheetNotifyKillActive(HWND hDlg, WPARAM wParam, LPARAM lParam);
        virtual INT_PTR OnSheetNotifyReset(HWND hDlg, WPARAM wParam, LPARAM lParam);
        virtual INT_PTR OnSheetNotifySetActive(HWND hDlg, WPARAM wParam, LPARAM lParam);
};

#endif // POLICY_MMC_SNAPIN


#ifdef PER_DIRECTORY_QUOTAS
//
// Folder property page.
//
class FolderPropPage : public DiskQuotaPropSheetExt
{
//
// This class has not been implemented.
// At some future date, we may implement per-directory quota management.
// If such support is required, look at class VolumePropPage.
// You should be able to provide a similar implementation only with
// directory-specific features.  All features common to volumes and
// directories are in class DiskQuotaPropSheetExt.
//
    public:
        //
        // Dialog Proc callback.
        //
        static INT_PTR APIENTRY DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
            { return FALSE; }
};


#endif // PER_DIRECTORY_QUOTAS




#endif // __DSKQUOTA_PROPSHEET_EXT_H

