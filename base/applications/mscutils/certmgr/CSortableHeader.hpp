/*
 * PROJECT:     ReactOS Certificate Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CSortableHeader definition
 * COPYRIGHT:   Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

struct CSortableHeader
{
  private:
    int m_LastHeaderID = 0;
    bool m_Ascending = true;

  public:
    struct SortContext
    {
        CSortableHeader *hdr;
        CListView *lvw;
        INT iSubItem;
    };

    static INT CALLBACK
    s_CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
    {
        SortContext *ctx = ((SortContext *)lParamSort);
        return ctx->hdr->CompareFunc(lParam1, lParam2, ctx);
    }

    INT
    CompareFunc(LPARAM lParam1, LPARAM lParam2, SortContext *ctx)
    {
        const INT iSubItem = ctx->iSubItem;
#define MAX_STR_LEN 256

        CStringW Item1, Item2;
        LVFINDINFOW IndexInfo;
        INT Index;

        IndexInfo.flags = LVFI_PARAM;

        IndexInfo.lParam = lParam1;
        Index = ctx->lvw->FindItem(-1, &IndexInfo);
        ctx->lvw->GetItemText(Index, iSubItem, Item1.GetBuffer(MAX_STR_LEN), MAX_STR_LEN);
        Item1.ReleaseBuffer();

        IndexInfo.lParam = lParam2;
        Index = ctx->lvw->FindItem(-1, &IndexInfo);
        ctx->lvw->GetItemText(Index, iSubItem, Item2.GetBuffer(MAX_STR_LEN), MAX_STR_LEN);
        Item2.ReleaseBuffer();

        return m_Ascending ? Item1.Compare(Item2) : Item2.Compare(Item1);
    }

    void
    ColumnClick(CListView &listView, LPNMLISTVIEW pnmv)
    {
        if ((listView.GetWindowLongPtr(GWL_STYLE) & ~LVS_NOSORTHEADER) == 0)
            return;

        ApplySorting(listView, pnmv->iSubItem);

        /* Save new values */
        m_LastHeaderID = pnmv->iSubItem;
        m_Ascending = !m_Ascending;
    }

    void
    ReapplySorting(CListView &listView)
    {
        if (m_LastHeaderID != -1)
        {
            ApplySorting(listView, m_LastHeaderID);
        }
    }

    void
    ApplySorting(CListView &listView, INT nHeaderID)
    {
        HWND hHeader = (HWND)listView.SendMessage(LVM_GETHEADER, 0, 0);
        HDITEMW hColumn;
        ZeroMemory(&hColumn, sizeof(hColumn));

        /* If the sorting column changed, remove the sorting style from the old column */
        if ((m_LastHeaderID != -1) && (m_LastHeaderID != nHeaderID))
        {
            m_Ascending = TRUE; // also reset sorting method to ascending
            hColumn.mask = HDI_FORMAT;
            Header_GetItem(hHeader, m_LastHeaderID, &hColumn);
            hColumn.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
            Header_SetItem(hHeader, m_LastHeaderID, &hColumn);
        }

        /* Set the sorting style to the new column */
        hColumn.mask = HDI_FORMAT;
        Header_GetItem(hHeader, nHeaderID, &hColumn);

        hColumn.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        hColumn.fmt |= (m_Ascending ? HDF_SORTUP : HDF_SORTDOWN);
        Header_SetItem(hHeader, nHeaderID, &hColumn);

        /* Sort the list, using the current values of nHeaderID and bIsAscending */
        SortContext ctx = {this, &listView, nHeaderID};
        listView.SortItems(s_CompareFunc, &ctx);
    }
};
