#include "pch.h"
#pragma hdrstop

#include "viewcols.h"
#include "cachview.h"

//
// Define static members of LVColumn.
//
CscObjDispInfo LVColumn::m_ObjDispInfo;    // Cached CSC object display info.
int            LVColumn::m_iLastItem = -1; // Index of last LV item updated.
CString        LVColumn::m_strItem;        // Item display string.


LRESULT
LVColumn::GetDispInfo(
    LV_DISPINFO *pdi
    ) const
{
    DBGASSERT((NULL != pdi));

    //
    // First cast the lParam back to the CscObject ptr that it is.
    //
    const CscObject *pObject = reinterpret_cast<const CscObject *>(pdi->item.lParam);
    if (NULL != pObject)
    {
        if (m_iLastItem != pdi->item.iItem)
        {
            //
            // Refresh cached row display information if requesting
            // for a new row.  Otherwise, we just use the information cached
            // from updating the previous columns in the row.
            //
            m_CscObjTree.GetObjDispInfo(pObject, &m_ObjDispInfo, ODI_ALL);
            m_iLastItem = pdi->item.iItem;
        }

        if (LVIF_TEXT & pdi->item.mask)
        {
            //
            // Retrieve the correct text from the cached display info.
            // This is a virtual function that will resolve to the proper
            // derived column object.  The double typecast is gross but it
            // prevents us from doing a CopyOnWrite with the CString object.
            //
            GetDispInfo_Text(*pObject, m_ObjDispInfo, &m_strItem);
            pdi->item.pszText = (LPTSTR)((LPCTSTR)m_strItem);
        }

        if (LVIF_IMAGE & pdi->item.mask)
        {
            //
            // Retrieve the correct image index for the item's state.
            // This is a virtual function that will resolve to the proper
            // derived column object.
            //
            pdi->item.iImage = GetDispInfo_Image(*pObject);
        }

        if (LVIF_STATE & pdi->item.mask)
        {
            //
            // Retrieve the overlay image if one exists.  Again,
            // GetDispInfo_ImageOverlay() is a virtual function.
            //
            pdi->item.stateMask = LVIS_OVERLAYMASK;

            int iImageOverlay = GetDispInfo_ImageOverlay(*pObject);
            if (iIMAGELIST_ICON_NOIMAGE != iImageOverlay)
            {
                pdi->item.state |= iImageOverlay;
            }
        }
    }
    return 0;
}


int
LVColPinned::GetDispInfo_Image(
    const CscObject& Object
    ) const
{
    return Object.IsPinned() ? iIMAGELIST_ICON_PIN : iIMAGELIST_ICON_UNPIN;
}


int 
LVColPinned::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    //
    // Table-driven approach to help sort perf as much as possible.
    //
    // 3D table:  p = "pinned", !p = "not pinned".
    //
    //       Ascending       Descending
    //       =============== ====================
    //       A(p)  A(!p)     A(p)   A(!p)
    //       --------------  --------------
    // B(p)   0     1          0      -1
    // B(!p) -1     0          1       0

    static const char rgDiffs[2][2][2] = {{{ 0,  1},
                                           {-1,  0}},
                                          {{ 0, -1},
                                           { 1,  0}}};

    return rgDiffs[int(bAscending)][int(b.IsPinned())][int(a.IsPinned())];
}


int
LVColEncrypted::GetDispInfo_Image(
    const CscObject& Object
    ) const
{
    return Object.IsEncrypted() ? iIMAGELIST_ICON_ENCRYPTED : iIMAGELIST_ICON_DECRYPTED;
}

int 
LVColEncrypted::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    //
    // Table-driven approach to help sort perf as much as possible.
    //
    // 3D table:  e = "encrypted", !e = "not encrypted".
    //
    //       Ascending       Descending
    //       =============== ====================
    //       A(e)  A(!e)     A(e)   A(!e)
    //       --------------  --------------
    // B(e)   0     1          0      -1
    // B(!e) -1     0          1       0

    static const char rgDiffs[2][2][2] = {{{ 0,  1},
                                           {-1,  0}},
                                          {{ 0, -1},
                                           { 1,  0}}};

    return rgDiffs[int(bAscending)][int(b.IsEncrypted())][int(a.IsEncrypted())];
}


int
LVColName::GetDispInfo_Image(
    const CscObject& object
    ) const
{
    int iImage = 0;
    switch(object.IsA())
    {
        case CscObject::File:
            iImage = object.GetIconImageIndex();
            if (-1 == iImage)
                iImage = iIMAGELIST_ICON_DOCUMENT;

            break;

        case CscObject::Folder:
            iImage = iIMAGELIST_ICON_FOLDER;
            break;

        default:
            break;
    }
    return iImage;
}


int 
LVColName::GetDispInfo_ImageOverlay(
    const CscObject& object
    ) const
{
    return object.NeedToSync() ? INDEXTOOVERLAYMASK(1) : iIMAGELIST_ICON_NOIMAGE;
}


void 
LVColName::GetDispInfo_Text(
    const CscObject& object,
    const CscObjDispInfo& di,
    CString *pstrText
    ) const
{
    DBGASSERT((NULL != pstrText));

    if (object.IsFolder())
        static_cast<const CscFolder&>(object).GetName(pstrText);
    else
        *pstrText = di.m_strFile;
}


void 
LVColFolder::GetDispInfo_Text(
    const CscObject& object, 
    const CscObjDispInfo& di, 
    CString *pstrText
    ) const
{ 
    DBGASSERT((NULL != pstrText)); 

    if (object.IsFolder())
    {
        const CscFolder *pParent = static_cast<const CscFolder *>(object.GetParent());
        pParent->GetDispInfo(&m_diA, ODI_PATH);
        *pstrText = m_diA.m_strPath;
    }
    else
        *pstrText = di.m_strPath; 

    if (pstrText->IsEmpty())
        *pstrText = m_strBackSlash;
}


int 
LVColName::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    //
    // First compare on object type.  Folders sort before files.
    //
    int diff = a.IsA() - b.IsA();
    if (0 == diff)
    {
        //
        // Both are either a folder or a file.
        //
        if (a.IsFolder())
        {
            //
            // Compare by folder name.
            //
            static_cast<const CscFolder&>(a).GetName(&m_diA.m_strFile);
            static_cast<const CscFolder&>(b).GetName(&m_diB.m_strFile);
        }            
        else
        {
            //
            // Compare by file name.
            //
            m_CscObjTree.GetObjDispInfo(&a, &m_diA, ODI_FILE);
            m_CscObjTree.GetObjDispInfo(&b, &m_diB, ODI_FILE);
        }
        diff = m_diA.m_strFile.CompareNoCase(m_diB.m_strFile);
    }

    return bAscending ? diff : -1 * diff;
}


int 
LVColServer::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    m_CscObjTree.GetObjDispInfo(&a, &m_diA, ODI_SERVER);
    m_CscObjTree.GetObjDispInfo(&b, &m_diB, ODI_SERVER);

    int diff = m_diA.m_strServer.CompareNoCase(m_diB.m_strServer);

    return bAscending ? diff : -1 * diff;
}


int 
LVColShare::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    m_CscObjTree.GetObjDispInfo(&a, &m_diA, ODI_SHARE);
    m_CscObjTree.GetObjDispInfo(&b, &m_diB, ODI_SHARE);

    int diff = m_diA.m_strShare.CompareNoCase(m_diB.m_strShare);

    return bAscending ? diff : -1 * diff;
}


int 
LVColShareWithImage::GetDispInfo_Image(
    const CscObject& Object
    ) const
{ 
    return Object.IsShareOnLine() ? iIMAGELIST_ICON_SHARE : iIMAGELIST_ICON_SHARE_NOCNX;
}


int 
LVColFolder::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    if (!a.IsFolder())
    {
        m_CscObjTree.GetObjDispInfo(&a, &m_diA, ODI_PATH);
    }
    else
    {
        const CscFolder *pParent = static_cast<const CscFolder *>(a.GetParent());
        pParent->GetDispInfo(&m_diA, ODI_PATH);
        if (m_diA.m_strPath.IsEmpty())
            m_diA.m_strPath = m_strBackSlash;
    }

    if (!b.IsFolder())
    {
        m_CscObjTree.GetObjDispInfo(&b, &m_diB, ODI_PATH);
    }
    else
    {
        const CscFolder *pParent = static_cast<const CscFolder *>(b.GetParent());
        pParent->GetDispInfo(&m_diB, ODI_PATH);
        if (m_diB.m_strPath.IsEmpty())
            m_diB.m_strPath = m_strBackSlash;
    }

    int diff = m_diA.m_strPath.CompareNoCase(m_diB.m_strPath);

    return bAscending ? diff : -1 * diff;
}


int 
LVColModified::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    int diff = 0;

    //
    // Sort order:
    //
    // If 'a' and 'b' have modified dates, sort by date.
    // If either 'a' or 'b' (XOR) have no modified date, the one without 
    // sorts first.
    // If both 'a' and 'b' have no modified date, the sort order is
    //      Share (no date), Folder, File
    //
    FILETIME ftA = a.GetFileTime();
    FILETIME ftB = b.GetFileTime();
    diff = CompareFileTime(&ftA, &ftB);

    if (0 == diff)
    {
        diff = (int)a.IsA() - (int)b.IsA();
    }

    return bAscending ? diff : -1 * diff;
}


int 
LVColSize::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    LONGLONG diff = 0;

    //
    // Sort order:
    //
    // If 'a' and 'b' have size info, sort by size.
    // If either 'a' or 'b' (XOR) have no size info, the one without 
    // sorts first.
    // If both 'a' and 'b' have no size info, the sort order is
    //      Share (no date), Folder, File
    //
    diff = a.GetFileSize() - b.GetFileSize();

    if (0 < diff)
        diff = 1;
    else if (0 == diff)
        diff = (int)a.IsA() - (int)b.IsA(); // Neither 'a' nor 'b' have time info.;
    else
        diff = -1;

    return (int)(bAscending ? diff : -1 * diff);
}




int 
LVColStaleReason::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    int diff = 0;

    //
    // Sort order:
    //
    // If 'a' and 'b' are files, sort by stale reason text string.
    // If either 'a' or 'b' (XOR) is not a file, the one that is not
    // sorts first.
    // If both 'a' and 'b' are not files, the sort order is
    //      Share (no date), Folder, File
    //
    if (a.IsFile() && b.IsFile())
    {
        //
        // This first block of code creates a static sort-order lookup table
        // for comparing files based on the stale reason.  The stale reason
        // column in the view must be sorted on lexical order of the reason 
        // text and not the reason code itself.  To avoid lengthy string 
        // compares of this rather verbose text, this lookup table will allow 
        // us to compare view entries based on lexical order without doing 
        // the actual string compares each time.  The string compares are only 
        // performed once when the lookup table is first created.  The final 
        // table has an entry for each reason code biased by the value of the 
        // first reason code value.  For example, the first element (index 0) 
        // corresponds to IDS_STALEREASON_NOTSTALE.  Each entry contains the 
        // relative sort order to the other entries.
        //
        const int IFIRST = IDS_STALEREASON_NOTSTALE;
        const int ILAST  = IDS_STALEREASON_SPARSE;
        //
        // rgReasonOrder is the final lookup table.  static so we only
        // initialize it once.
        //
        static int rgReasonOrder[ILAST - IFIRST + 1] = { -1 };
        if (-1 == rgReasonOrder[0])
        {
            //
            // A temporary map of reason text and reason codes
            // used for building the order lookup table.
            //
            struct Map
            {
                Map(void) : iCode(-1) { }  // ctor req'd to create array.
                CString strDesc;
                int     iCode;

            } map[ARRAYSIZE(rgReasonOrder)];

            //
            // First build up an array of reason strings and codes sorted 
            // lexically on the reason description.
            //
            for (int i = 0; i < ARRAYSIZE(rgReasonOrder); i++)
            {
                map[i].strDesc.Format(Viewer::g_hInstance, IFIRST + i);
                map[i].iCode = IFIRST + i;
                int j = i;
                while(j > 0 && map[j].strDesc < map[j-1].strDesc)
                {
                    SWAP(map[j].strDesc, map[j-1].strDesc);
                    SWAP(map[j].iCode,   map[j-1].iCode);
                    j--;
                }
                //
                // See entries with number that sorts higher than any
                // order number we might be placing in it.
                //
                rgReasonOrder[i] = ARRAYSIZE(rgReasonOrder);
            }
            //
            // Now build the final lookup table.  Each entry index represents
            // a reason code (ordered by reason code) and each entry value
            // is the relative lexical sort order of the reason text.
            // For example, the entry for IDS_STALEREASON_NONE will be in slot 0
            // (because it's first in the list of stale reason codes) and it will 
            // contain the value 0 because " " sorts before any other text strings.
            //
            for (i = 0; i < ARRAYSIZE(rgReasonOrder); i++)
            {
                rgReasonOrder[i] = map[i].iCode - IFIRST;
                int j = i;
                while(j > 0 && rgReasonOrder[j] < rgReasonOrder[j-1])
                {
                    SWAP(rgReasonOrder[j], rgReasonOrder[j-1]);
                    j--;
                }
            }
        }
            
        const CscFile& fileA = static_cast<const CscFile&>(a);
        const CscFile& fileB = static_cast<const CscFile&>(b);
        DBGASSERT((0 <= fileA.GetStaleReasonCode() - IFIRST));
        DBGASSERT((ARRAYSIZE(rgReasonOrder) > fileA.GetStaleReasonCode() - IFIRST));
        DBGASSERT((0 <= fileB.GetStaleReasonCode() - IFIRST));
        DBGASSERT((ARRAYSIZE(rgReasonOrder) > fileB.GetStaleReasonCode() - IFIRST));

        diff = rgReasonOrder[fileA.GetStaleReasonCode() - IFIRST] - 
               rgReasonOrder[fileB.GetStaleReasonCode() - IFIRST];
    }
    else if (!a.IsFile() && !b.IsFile())
    {
        diff = (int)a.IsA() - (int)b.IsA();
    }
    else
    {
        if (!a.IsFile())
            diff = 1;
        else
            diff = -1;
    }

    return bAscending ? diff : -1 * diff;
}


int 
LVColShareStatus::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    int diff = 0;


    DBGASSERT((a.IsShare()));
    DBGASSERT((b.IsShare()));

    const CscShare& shareA = static_cast<const CscShare&>(a);
    const CscShare& shareB = static_cast<const CscShare&>(b);

    shareA.GetStatusText(&m_strStatus[0]);
    shareB.GetStatusText(&m_strStatus[1]);

    diff = m_strStatus[0].CompareNoCase(m_strStatus[1]);

    return bAscending ? diff : -1 * diff;
}


void 
LVColObjectCount::GetDispInfo_Text(
    const CscObject& object, 
    const CscObjDispInfo& di, 
    CString *pstrText
    ) const
{ 
    DBGASSERT((NULL != pstrText));
    pstrText->FormatNumber(static_cast<const CscShare &>(object).GetFileCount());
}


int 
LVColObjectCount::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    int diff = 0;


    DBGASSERT((a.IsShare()));
    DBGASSERT((b.IsShare()));

    const CscShare& shareA = static_cast<const CscShare&>(a);
    const CscShare& shareB = static_cast<const CscShare&>(b);

    diff = shareA.GetFileCount() - shareB.GetFileCount();

    return bAscending ? diff : -1 * diff;
}

void 
LVColPinnedCount::GetDispInfo_Text(
    const CscObject& object, 
    const CscObjDispInfo& di, 
    CString *pstrText
    ) const
{ 
    DBGASSERT((NULL != pstrText)); 
    pstrText->FormatNumber(static_cast<const CscShare &>(object).GetPinnedFileCount());
}


int 
LVColPinnedCount::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    int diff = 0;


    DBGASSERT((a.IsShare()));
    DBGASSERT((b.IsShare()));

    const CscShare& shareA = static_cast<const CscShare&>(a);
    const CscShare& shareB = static_cast<const CscShare&>(b);

    diff = (shareA.GetPinnedFileCount() - shareB.GetPinnedFileCount());

    return bAscending ? diff : -1 * diff;
}


void 
LVColUpdateCount::GetDispInfo_Text(
    const CscObject& object, 
    const CscObjDispInfo& di, 
    CString *pstrText
    ) const
{ 
    DBGASSERT((NULL != pstrText)); 
    pstrText->FormatNumber(static_cast<const CscShare &>(object).GetStaleFileCount());
}


int 
LVColUpdateCount::CompareItems(
    const CscObject& a,
    const CscObject& b,
    bool bAscending
    ) const
{
    int diff = 0;


    DBGASSERT((a.IsShare()));
    DBGASSERT((b.IsShare()));

    const CscShare& shareA = static_cast<const CscShare&>(a);
    const CscShare& shareB = static_cast<const CscShare&>(b);

    diff = shareA.GetStaleFileCount() - shareB.GetStaleFileCount();

    return bAscending ? diff : -1 * diff;
}
