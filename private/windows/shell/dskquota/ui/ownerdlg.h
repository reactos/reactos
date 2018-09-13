#ifndef _INC_DSKQUOTA_OWNERDLG_H
#define _INC_DSKQUOTA_OWNERDLG_H

#ifndef _INC_DSKQUOTA_PRIVATE_H
#   include "private.h"
#endif

#ifndef _INC_DSKQUOTA_OWNERLST_H
#   include "ownerlst.h"
#endif

class CFileOwnerDialog
{
    public:
        CFileOwnerDialog(HINSTANCE hInstance,
                         HWND hwndParent,
                         LPCTSTR pszVolumeRoot,
                         const CArray<IDiskQuotaUser *>& rgpOwners);

        ~CFileOwnerDialog(void) { }

        INT_PTR Run(void);

    private:
        //
        // Code in ownerdlg.cpp relies on the values and order of these
        // enumerated constants.
        //
        enum { iLVSUBITEM_FILE = 0,
               iLVSUBITEM_FOLDER,
               iLVSUBITEM_OWNER
             };

        HINSTANCE  m_hInstance;
        HWND       m_hwndParent;
        HWND       m_hwndDlg;
        HWND       m_hwndLV;
        HWND       m_hwndOwnerCombo;
        HWND       m_hwndEditMoveTo;
        int        m_iLastColSorted;
        CPath      m_strVolumeRoot;
        const CArray<IDiskQuotaUser *>& m_rgpOwners; // Ref to existing list.
        COwnerList m_OwnerList;
        bool       m_bSortAscending;

        static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnInitDialog(HWND hwnd);
        INT_PTR OnDestroy(HWND hwnd);
        INT_PTR OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
        INT_PTR OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam);
        INT_PTR OnContextMenu(HWND hwndItem, int xPos, int yPos);
        void OnLVN_GetDispInfo(LV_DISPINFO *plvdi);
        void OnLVN_ColumnClick(NM_LISTVIEW *pnmlv);
        void OnLVN_ItemChanged(NM_LISTVIEW *pnmlv);
        void OnLVN_KeyDown(NMLVKEYDOWN *plvkd);
        void InitializeList(const COwnerList& fol, HWND hwndList);
        void FillListView(const COwnerList& fol, HWND hwndList, int iOwner = -1);
        void InitializeOwnerCombo(const COwnerList& fol, HWND hwndCombo);
        void CreateListColumns(HWND hwndList, bool bShowOwner = true);
        bool IsSameVolume(LPCTSTR pszRoot1, LPCTSTR pszRoot2);
        bool BrowseForFolder(HWND hwndParent, CString *pstrFolder);
        bool ShouldEnableButton(UINT idBtn);
        void RemoveListViewItems(HWND hwndLV, const CArray<COwnerListItemHandle>& rgItemHandles);
        void BuildListOfSelectedFiles(HWND hwndLV, DblNulTermList *pList, CArray<COwnerListItemHandle> *prgItemHandles);
        void MoveSelectedFiles(HWND hwndLV, LPCTSTR pszDest);
        void DeleteSelectedFiles(HWND hwndLV);
        void FocusOnSomethingInListview(HWND hwndLV);
        int FindItemFromHandle(HWND hwndLV, const COwnerListItemHandle& handle);
        HRESULT TakeOwnershipOfSelectedFiles(HWND hwndLV);
        HRESULT GetOwnershipSid(array_autoptr<BYTE> *ptrSid);
        static int CALLBACK CompareLVItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
        static int CALLBACK BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

        HRESULT
        BuildFileOwnerList(
            LPCTSTR pszVolumeRoot,
            const CArray<IDiskQuotaUser *>& rgpOwners,
            COwnerList *pOwnerList);

        HRESULT
        AddFilesToOwnerList(
            LPCTSTR pszVolumeRoot,
            HANDLE hVolumeRoot,
            IDiskQuotaUser *pOwner,
            COwnerList *pOwnerList);

        NTSTATUS
        OpenNtObject (
            LPCWSTR pszFile,
            HANDLE RelatedObject,
            ULONG CreateOptions,
            ULONG DesiredAccess,
            ULONG ShareAccess,
            ULONG CreateDisposition,
            HANDLE *ph);

};



#endif // _INC_DSKQUOTA_OWNERDLG_H

