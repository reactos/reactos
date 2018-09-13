#ifndef _INC_CSCVIEW_VIEWCOLS_H
#define _INC_CSCVIEW_VIEWCOLS_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_COMMCTRL
#   include <commctrl.h>
#endif

#ifndef _INC_CSCVIEW_OBJTREE_H
#   include "objtree.h"
#endif


class CacheView; // fwd decl.

//
// Define some names for indexes into the listview's image list.
//
enum ColumnImages { iIMAGELIST_ICON_NOIMAGE = -1,
                    iIMAGELIST_ICON_DOCUMENT = 0,
                    iIMAGELIST_ICON_SHARE,
                    iIMAGELIST_ICON_SHARE_NOCNX,
                    iIMAGELIST_ICON_FOLDER,
                    iIMAGELIST_ICON_STALE,
                    iIMAGELIST_ICON_PIN,
                    iIMAGELIST_ICON_UNPIN,
                    iIMAGELIST_ICON_OVERLAY_STALE,
                    iIMAGELIST_ICON_ENCRYPTED,
                    iIMAGELIST_ICON_DECRYPTED };

class LVColumn
{
    public:
        LVColumn(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idTitleStr, int iHeaderImage)
            : m_view(view),
              m_CscObjTree(tree),
              m_fmt(fmt),
              m_cx(cx),
              m_idTitleStr(idTitleStr),
              m_iHeaderImage(iHeaderImage) { }

        virtual ~LVColumn(void) { }

        int GetFormat(void) const throw()
            { return m_fmt; }
        int GetWidth(void) const throw()
            { return m_cx; }
        DWORD GetTitleStrId(void) const throw()
            { return m_idTitleStr; }
        bool HasImage(void) const throw()
            { return (-1 != m_iHeaderImage); }
        int GetHeaderImageIndex(void) const throw()
            { return m_iHeaderImage; }
        static void ResetCache(void) throw()
            { LVColumn::m_iLastItem = -1; }

        //
        // Called in response to LVN_GETDISPINFO
        //
        LRESULT GetDispInfo(LV_DISPINFO *pdi) const;

        //
        // Called when items need to be sorted.
        //
        virtual int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const = 0;

    protected:
        //
        // Return a constant reference to the cached object display information.
        //
        const CscObjDispInfo& GetObjDispInfo(void) const
            { return m_ObjDispInfo; }

        //
        // Get the column-specific text display info.
        // Default is "" (no text).
        //
        virtual void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const
            { DBGASSERT((NULL != pstrText)); pstrText->Empty(); }

        //
        // Get the column-specific image display info.
        // Default is -1 (no image).
        //
        virtual int GetDispInfo_Image(const CscObject& object) const
            { return iIMAGELIST_ICON_NOIMAGE; }

        //
        // Get the overlay mask image index.
        // Default is -1 (no image).
        //
        virtual int GetDispInfo_ImageOverlay(const CscObject& object) const
            { return iIMAGELIST_ICON_NOIMAGE; }

        const CscObjTree& m_CscObjTree;  // Reference to tree of CSC database info.

        //
        // These two members are used during column sorting.
        // The item comparision functions (for each subclass) for some of the
        // column types need to compare display text information such as file
        // or path name.  To retrieve this information from the CscObjTree, 
        // a CscObjDispInfo object is required.  Providing these as members
        // avoids the creation and destruction each time a comparison function
        // is called (which can be often).  Providing these as members of the
        // LVColumn class (instead of local variables in the sort functions)
        // reduced sorting times by half.
        //
        mutable CscObjDispInfo m_diA;  // For comparisons.
        mutable CscObjDispInfo m_diB;  // For comparisons.

        const CacheView& m_view; 

    private:
        int m_fmt;          // LVCFMT_xxxxx constant.
        int m_cx;           // Column width (pixels).
        int m_idTitleStr;   // Title text string resource ID.
        int m_iHeaderImage; // Column's header image.

        //
        // Cache the display information for an item.
        // This information is refreshed in GetDispInfo.
        //
        static CscObjDispInfo m_ObjDispInfo;   // Cached CSC object display info.
        static int            m_iLastItem;     // Index of last LV item updated.
        static CString        m_strItem;       // Item display string.
};



class LVColPinned : public LVColumn
{
    public:
        LVColPinned(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        virtual int GetDispInfo_Image(const CscObject& object) const;
        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;
};

class LVColEncrypted : public LVColumn
{
    public:
        LVColEncrypted(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        virtual int GetDispInfo_Image(const CscObject& object) const;
        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;
};

class LVColName : public LVColumn
{
    public:
        LVColName(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        int GetDispInfo_Image(const CscObject& Object) const;
        int GetDispInfo_ImageOverlay(const CscObject& object) const;
        void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const;
        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;
};


class LVColServer : public LVColumn
{
    public:
        LVColServer(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const
            { DBGASSERT((NULL != pstrText)); *pstrText = di.m_strServer; }

        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;
};


class LVColShare : public LVColumn
{
    public:
        LVColShare(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const
            { DBGASSERT((NULL != pstrText)); *pstrText = di.m_strShareDisplayName; }

        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;
};


class LVColShareWithImage : public LVColShare
{
    public:
        LVColShareWithImage(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColShare(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        int GetDispInfo_Image(const CscObject& Object) const;
};


class LVColShareStatus : public LVColumn
{
    public:
        LVColShareStatus(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const
            { DBGASSERT((NULL != pstrText)); *pstrText = di.m_strShareStatus; }

        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;

    private:
        mutable CString m_strStatus[2];
};

class LVColFolder : public LVColumn
{
    public:
        LVColFolder(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage),
              m_strBackSlash(TEXT("\\")) { }

        void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const;

        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;

    private:
        const CString m_strBackSlash;

};


class LVColModified : public LVColumn
{
    public:
        LVColModified(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const
            { DBGASSERT((NULL != pstrText)); *pstrText = di.m_strFileTime; }

        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;
};


class LVColSize : public LVColumn
{
    public:
        LVColSize(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const
            { DBGASSERT((NULL != pstrText)); *pstrText = di.m_strFileSize; }

        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;
};


class LVColStaleReason : public LVColumn
{
    public:
        LVColStaleReason(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const
            { DBGASSERT((NULL != pstrText)); *pstrText = di.m_strStaleReason; }

        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;

    private:
        mutable CString m_strReason[2];
};

class LVColObjectCount : public LVColumn
{
    public:
        LVColObjectCount(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const;

        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;
};

class LVColPinnedCount : public LVColumn
{
    public:
        LVColPinnedCount(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const;

        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;
};

class LVColUpdateCount : public LVColumn
{
    public:
        LVColUpdateCount(const CacheView& view, const CscObjTree& tree, int fmt, int cx, int idMsgText, int iHeaderImage = iIMAGELIST_ICON_NOIMAGE)
            : LVColumn(view, tree, fmt, cx, idMsgText, iHeaderImage) { }

        void GetDispInfo_Text(const CscObject& object, const CscObjDispInfo& di, CString *pstrText) const;

        int CompareItems(const CscObject& a, const CscObject& b, bool bAscending) const;
};

#endif // _INC_CSCVIEW_VIEWCOLS_H

