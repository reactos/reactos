#ifndef _INC_DSKQUOTA_DETAILS_H
#define _INC_DSKQUOTA_DETAILS_H
///////////////////////////////////////////////////////////////////////////////
/*  File: details.h

    Description: Declaration for class DetailsView.
        This is a complex class but don't be intimidated by it.
        Much of the functionality has been layered in private subclasses
        so that the scope of any individual piece is minimized.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/20/96    Initial creation.                                    BrianAu
    12/06/96    Removed INLINE from message handler methods.         BrianAu
                It's just too hard to debug when they're inline.
    05/28/97    Major changes.                                       BrianAu
                - Added "User Finder".
                - Added promotion of selected item to front of
                  name resolution queue.
                - Improved name resolution status reporting through
                  listview.
                - Moved drag/drop and report generation code
                  from dragdrop.cpp and reptgen.cpp into the
                  DetailsView class.  DetailsView now implements
                  IDataObject, IDropSource and IDropTarget instead
                  of deferring implementation to secondary objects.
                  dragdrop.cpp and reptgen.cpp have been dropped
                  from the project.
                - Added support for CF_HDROP and private import/
                  export clipboard formats.
                - Added import/export functionality.
    07/28/97    Removed export support for CF_HDROP.  Replaced       BrianAu
                with FileContents and FileGroupDescriptor.  Import
                from CF_HDROP is still supported.
                Added Import Source object hierarchy.
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_DSKQUOTA_H
#   include "dskquota.h"
#endif
#ifndef _INC_DSKQUOTA_CONTROL_H
#   include "control.h"
#endif
#ifndef _INC_DSKQUOTA_UNDO_H
#   include "undo.h"
#endif
#ifndef _INC_DSKQUOTA_FORMAT_H
#   include "format.h"
#endif
#ifndef _INC_DSKQUOTA_PROGRESS_H
#   include "progress.h"
#endif

//
// Custom messages for this implementation.
//
#define WM_MAINWINDOW_CREATED         (WM_USER + 1)
#define WM_ADD_USER_TO_DETAILS_VIEW   (WM_USER + 2)
#define WM_DEL_USER_FROM_DETAILS_VIEW (WM_USER + 3)


//
// Structure containing column definition data for the listview.
//
struct DV_COLDATA
{
    int fmt;
    int cx;
    DWORD idMsgText;
    int iColId;
};


//
// Listview report item structure.
// Used for obtaining text/numeric data for a given item in the listview
// for purposes of generating a drag-drop data source.
//
typedef struct
{
    DWORD  fType;
    LPTSTR pszText;
    UINT   cchMaxText;
    DWORD  dwValue;
    double dblValue;
} LV_REPORT_ITEM, *PLV_REPORT_ITEM;

//
// Listview Report Item (LVRI) type constants (fType)
// These indicate what type of data is requested for a LV_REPORT_ITEM and
// also what type of data is provided in LV_REPORT_ITEM.  A caller of
// DetailsView::GetReportItem provides an LV_REPORT_ITEM that acts as the
// communication mechanism between the caller and the DetailsView.  The
// caller fills in the fType member indicating what format of information
// is desired for that row/col item.  The request may be any one of the
// following constants:
//
//      LVRI_TEXT   = Would like data in text format if possible.
//      LVRI_INT    = Would like data in integer format if possible.
//      LVRI_REAL   = Would like data in floating point format if possible.
//      LVRI_NUMBER = Would like data in either INT or REAL format if possible.
//
// This value in fType is merely a hint.  If the data can't be provided in the
// requested format, the next best format is supplied.  Upon return, the fType
// flag may be modified to indicate the actual format of the data returned.
// This value may be either LVRI_TEXT, LVRI_INT or LVRI_REAL.  LVRI_NUMBER is
// only used for hinting by the caller.
//
const DWORD LVRI_TEXT   = 0x00000001;
const DWORD LVRI_INT    = 0x00000002;
const DWORD LVRI_REAL   = 0x00000004;
const DWORD LVRI_NUMBER = (LVRI_INT | LVRI_REAL);

//
// Structure of "ListViewState" information stored in registry per-user.
// Note that we include the structure size and the screen width/height
// to validate the information when we read it from the registry.  If the
// structure size has changed, we don't trust the data and use defaults.
// If the screen size has changed, we use defaults.
//
//
//
// WARNING: I really don't like this but...
//          The size of the rgcxCol[] member must be at least as large
//          as the value of DetailsView::idCol_Last.  Because of the
//          order dependencies of the LV_STATE_INFO and DetailsView
//          structures, I can't use idCol_Last in this declaration.
//          If you have to add a new column and change the value of
//          idCol_Last, make sure the size of rgcxCol[] is adjusted
//          appropriately.  Also adjust rgColIndices[].
//
typedef struct
{
    WORD cb;                   // Count of bytes in structure.
    WORD wVersion;             // Version of state info (for upgrades).
    LONG cxScreen;             // Screen width.
    LONG cyScreen;             // Screen height.
    LONG cx;                   // Width of window (pixels).
    LONG cy;                   // Height of window (pixels).
    WORD fToolBar       :  1;  // Toolbar visible?
    WORD fStatusBar     :  1;  // Status bar visible?
    WORD fShowFolder    :  1;  // Folder column visible?
    WORD iLastColSorted :  4;  // Current sort column.
    WORD fSortDirection :  1;  // 0 = Ascending, 1 = Descending.
    WORD fReserved      :  8;  // Unused bits.
    INT  rgcxCol[8];           // Width of each column (pixels).
    INT  rgColIndices[8];      // Order of subitems in listview.

} LV_STATE_INFO, *PLV_STATE_INFO;

//
// Increment this if you make a change that causes problems with
// state info saved for existing users.  It will cause us to invalidate
// any existing state information and to use defaults.  It may cancel
// any user's existing preferences but at least the view will look OK.
//
const WORD wLV_STATE_INFO_VERSION = 3;

//
// This class maps our column ids (idCol_XXXX) to a listview column
// index (SubItem).
//
class ColumnMap
{
    private:
        INT *m_pMap;
        UINT m_cMapSize;

        //
        // Prevent copying.
        //
        ColumnMap(const ColumnMap&);
        void operator = (const ColumnMap&);

    public:
        ColumnMap(UINT cMapSize);
        ~ColumnMap(VOID);

        INT SubItemToId(INT iSubItem) const;
        INT IdToSubItem(INT iColId) const;
        VOID RemoveId(INT iSubItem);
        VOID InsertId(INT iSubItem, INT iColId);
};


class DetailsView : public IDiskQuotaEvents,
                    public IDropSource,
                    public IDropTarget,
                    public IDataObject
{
    private:
        //
        // DetailsView::Finder ------------------------------------------------
        //
        //
        // This class implements the "find a user" feature.
        // 1. "Attaches" to the "find" combo box in the toolbar by subclassing
        //     that combo box window.
        // 2. Invokes the "Find User" dialog on command.
        // 3. Repositions the listview highlight bar on a user if found.
        // 4. Maintains an MRU list for populating the toolbar and dialog
        //    combo boxes.
        //
        class Finder
        {
            public:
                Finder(DetailsView& DetailsView, INT cMaxMru);
                VOID ConnectToolbarCombo(HWND hwndToolbarCombo);
                VOID InvokeFindDialog(HWND hwndParent);

                static INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
                static LRESULT CALLBACK ToolbarComboSubClassWndProc(HWND, UINT, WPARAM, LPARAM);

            private:
                HWND m_hwndToolbarCombo;    // Combo box in toolbar.
                INT  m_cMaxComboEntries;    // Max entries allowed in combo MRU.
                DetailsView& m_DetailsView; // Reference to associated details view.
                WNDPROC m_pfnOldToolbarComboWndProc; // Saved wnd proc address.

                VOID AddNameToCombo(HWND hwndCombo, LPCTSTR pszName, INT cMaxEntries);
                BOOL UserNameEntered(HWND hwndCombo);
                VOID FillDialogCombo(HWND hwndComboSrc, HWND hwndComboDest);

                //
                // Prevent copy.
                //
                Finder(const Finder& rhs);
                Finder& operator = (const Finder& rhs);
        };

        //
        // DetailsView::Importer ----------------------------------------------
        //
        class Importer
        {
            public:
                Importer(DetailsView& DV);
                ~Importer(VOID);

                HRESULT Import(IDataObject *pIDataObject);
                HRESULT Import(const FORMATETC& fmt, const STGMEDIUM& medium);
                HRESULT Import(LPCTSTR pszFilePath);
                HRESULT Import(HDROP hDrop);

            private:
                //
                // DetailsView::Importer::AnySource ---------------------------
                //
                // This small hierarchy of "Source" classes is here to insulate
                // the import process from the import source.  There are two
                // basic forms of import source data; OLE stream and memory-
                // mapped file.  So that we only have one function that actually
                // contains the import logic, this layer of abstraction insulates
                // that import function from any differences between streams
                // and simple memory blocks.
                // Instead of calling Import(pStream) or Import(pbBlock), a
                // client uses Import(Source(pIStream)) or Import(Source(pbBlock)).
                // The Source object uses the virtual constructor technique to
                // create the correct object for the input source.  Each
                // descendant of AnySource implements the single Read() function
                // to read data from it's specific source.
                //
                class AnySource
                {
                    public:
                        AnySource(VOID) { }
                        virtual ~AnySource(VOID) { }
                        virtual HRESULT Read(LPVOID pvOut, ULONG cb, ULONG *pcbRead) = 0;

                    private:
                        //
                        // Prevent copy.
                        //
                        AnySource(const AnySource& rhs);
                        AnySource& operator = (const AnySource& rhs);
                };

                //
                // DetailsView::Importer::StreamSource ------------------------
                //
                class StreamSource : public AnySource
                {
                    public:
                        StreamSource(IStream *pStm);
                        virtual ~StreamSource(VOID);

                        virtual HRESULT Read(LPVOID pvOut, ULONG cb, ULONG *pcbRead);

                    private:
                        IStream *m_pStm;

                        //
                        // Prevent copy.
                        //
                        StreamSource(const StreamSource& rhs);
                        StreamSource& operator = (const StreamSource& rhs);
                };

                //
                // DetailsView::Importer::MemorySource ------------------------
                //
                class MemorySource : public AnySource
                {
                    public:
                        MemorySource(LPBYTE pb, ULONG cbMax);
                        virtual ~MemorySource(VOID) { };

                        virtual HRESULT Read(LPVOID pvOut, ULONG cb, ULONG *pcbRead);

                    private:
                        LPBYTE m_pb;
                        ULONG  m_cbMax;

                        //
                        // Prevent copy.
                        //
                        MemorySource(const MemorySource& rhs);
                        MemorySource& operator = (const MemorySource& rhs);
                };

                //
                // DetailsView::Importer::Source ------------------------------
                //
                class Source
                {
                    public:
                        Source(IStream *pStm);
                        Source(LPBYTE pb, ULONG cbMax);

                        virtual ~Source(VOID);

                        virtual HRESULT Read(LPVOID pvOut, ULONG cb, ULONG *pcbRead);

                    private:
                        AnySource *m_pTheSource;

                        //
                        // Prevent copy.
                        //
                        Source(const Source& rhs);
                        Source& operator = (const Source& rhs);
                };


                //
                // These two import functions are the real workers.
                // All other import functions eventually end up at
                // Import(Source& ) which calls Import(pbSid, Threshold, Limit)
                // to import each user record.
                //
                HRESULT Import(Source& source);
                HRESULT Import(LPBYTE pbSid, LONGLONG llQuotaThreshold,
                                             LONGLONG llQuotaLimit);

                VOID Destroy(VOID);
                HWND GetTopmostWindow(VOID);

                DetailsView&   m_DV;
                BOOL           m_bUserCancelled;   // User cancelled import.
                BOOL           m_bPromptOnReplace; // Prompt user when replacing record?
                ProgressDialog m_dlgProgress;      // Progress dialog.
                HWND           m_hwndParent;       // Parent HWND for any UI elements.
                INT            m_cImported;        // Number of records imported.

                //
                // Prevent copy.
                //
                Importer(const Importer& rhs);
                Importer& operator = (const Importer& rhs);
        };

        //
        // DetailsView::DataObject --------------------------------------------
        //
        class DataObject
        {
            public:
                DataObject(DetailsView& DV);
                ~DataObject(VOID);

                HRESULT IsFormatSupported(FORMATETC *pFormatEtc);
                HRESULT RenderData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium);

                static VOID SetFormatEtc(FORMATETC& fe,
                                         CLIPFORMAT cfFormat,
                                         DWORD tymed,
                                         DWORD dwAspect = DVASPECT_CONTENT,
                                         DVTARGETDEVICE *ptd = NULL,
                                         LONG lindex = -1);

                static LPSTR WideToAnsi(LPCWSTR pszTextW);

                static const INT   CF_FORMATS_SUPPORTED;
                static LPCWSTR     SZ_EXPORT_STREAM_NAME;
                static LPCTSTR     SZ_EXPORT_CF_NAME;
                static const DWORD EXPORT_STREAM_VERSION;

                LPFORMATETC  m_rgFormats;            // Array of supported formats.
                DWORD        m_cFormats;             // Number of supported formats.
                static CLIPFORMAT m_CF_Csv;                // Comma-separated fields format.
                static CLIPFORMAT m_CF_RichText;           // RTF format.
                static CLIPFORMAT m_CF_NtDiskQuotaExport;  // Internal fmt for import/export.
                static CLIPFORMAT m_CF_FileGroupDescriptor;// Used by shell for drop to folder.
                static CLIPFORMAT m_CF_FileContents;       // Used by shell for drop to folder.

            private:
                //
                // DetailsView::DataObject::Renderer --------------------------
                //
                class Renderer
                {
                    protected:
                        //
                        // DetailsView::DataObject::Renderer::Stream ----------
                        //
                        class Stream
                        {
                            private:
                                IStream *m_pStm;

#ifdef CLIPBOARD_DEBUG_OUTPUT

                                IStorage *m_pStgDbgOut; // For debugging clipboard output.
                                IStream  *m_pStmDbgOut; // For debugging clipboard output.

#endif  // CLIPBOARD_DEBUG_OUTPUT
                                //
                                // Prevent copy.
                                //
                                Stream(const Stream& rhs);
                                Stream& operator = (const Stream& rhs);

                            public:
                                Stream(IStream *pStm = NULL);
                                ~Stream(VOID);

                                VOID SetStream(IStream *pStm);
                                IStream *GetStream(VOID)
                                    { return m_pStm; }

                                VOID Write(LPBYTE pbData, UINT cbData);
                                VOID Write(LPCSTR pszTextA);
                                VOID Write(LPCWSTR pszTextW);
                                VOID Write(BYTE bData);
                                VOID Write(CHAR chDataA);
                                VOID Write(WCHAR chDataW);
                                VOID Write(DWORD dwData);
                                VOID Write(double dblData);
                        };


                        DetailsView& m_DV;   // Details view is source of data.
                        Stream       m_Stm;  // Stream on which report is writtn.

                        virtual VOID Begin(INT cRows, INT cCols) { }
                        virtual VOID AddTitle(LPCWSTR pszTitleW) { }
                        virtual VOID BeginHeaders(VOID) { }
                        virtual VOID AddHeader(LPCWSTR pszHeaderW) { }
                        virtual VOID AddHeaderSep(VOID) { }
                        virtual VOID EndHeaders(VOID) { }
                        virtual VOID BeginRow(VOID) { }
                        virtual VOID AddRowColData(INT iRow, INT idCol) { }
                        virtual VOID AddRowColSep(VOID) { }
                        virtual VOID EndRow(VOID) { }
                        virtual VOID End(VOID) { }

                        //
                        // Prevent copy.
                        //
                        Renderer(const Renderer& rhs);
                        Renderer& operator = (const Renderer& rhs);


                    public:
                        Renderer(DetailsView& DV)
                            : m_DV(DV) { }

                        virtual ~Renderer(VOID) { }

                        virtual VOID Render(IStream *pStm);
                };

                //
                // DetailsView::DataObject::Renderer_UNICODETEXT --------------
                //
                class Renderer_UNICODETEXT : public Renderer
                {
                    private:
                        //
                        // Prevent copy.
                        //
                        Renderer_UNICODETEXT(const Renderer_UNICODETEXT& rhs);
                        Renderer_UNICODETEXT& operator = (const Renderer_UNICODETEXT& rhs);

                    protected:
                        virtual VOID AddTitle(LPCWSTR pszTitleW);

                        virtual VOID AddHeader(LPCWSTR pszHeaderW)
                            { m_Stm.Write(pszHeaderW); }

                        virtual VOID AddHeaderSep(VOID)
                            { m_Stm.Write(TEXT('\t')); }

                        virtual VOID EndHeaders(VOID)
                            { m_Stm.Write(TEXT('\n')); }

                        virtual VOID AddRowColData(INT iRow, INT idCol);

                        virtual VOID AddRowColSep(VOID)
                            { m_Stm.Write(TEXT('\t')); }

                        virtual VOID EndRow(VOID)
                            { m_Stm.Write(TEXT('\n')); }

                    public:
                        Renderer_UNICODETEXT(DetailsView& DV)
                            : Renderer(DV) { }

                        virtual ~Renderer_UNICODETEXT(VOID) { }
                };

                //
                // DetailsView::DataObject::Renderer_TEXT ---------------------
                //
                class Renderer_TEXT : public Renderer_UNICODETEXT
                {
                    private:
                        //
                        // Prevent copy.
                        //
                        Renderer_TEXT(const Renderer_TEXT& rhs);
                        Renderer_TEXT& operator = (const Renderer_TEXT& rhs);

                    protected:
                        virtual VOID AddTitle(LPCWSTR pszTitleW);

                        virtual VOID AddHeader(LPCWSTR pszHeaderW);

                        virtual VOID AddHeaderSep(VOID)
                            { m_Stm.Write('\t'); }

                        virtual VOID EndHeaders(VOID)
                            { m_Stm.Write('\n'); }

                        virtual VOID AddRowColData(INT iRow, INT idCol);

                        virtual VOID AddRowColSep(VOID)
                            { m_Stm.Write('\t'); }

                        virtual VOID EndRow(VOID)
                            { m_Stm.Write('\n'); }

                    public:
                        Renderer_TEXT(DetailsView& DV)
                            : Renderer_UNICODETEXT(DV) { }

                        virtual ~Renderer_TEXT(VOID) { }
                };


                //
                // DetailsView::DataObject::Renderer_Csv ----------------------
                //
                class Renderer_Csv : public Renderer_TEXT
                {
                    private:
                        //
                        // Prevent copy.
                        //
                        Renderer_Csv(const Renderer_Csv& rhs);
                        Renderer_Csv& operator = (const Renderer_Csv& rhs);

                    protected:
                        virtual VOID AddHeaderSep(VOID)
                            { m_Stm.Write(','); }

                        virtual VOID AddRowColSep(VOID)
                            { m_Stm.Write(','); }

                    public:
                        Renderer_Csv(DetailsView& DV)
                            : Renderer_TEXT(DV) { }

                        virtual ~Renderer_Csv(VOID) { }
                };

                //
                // DetailsView::DataObject::Renderer_RTF ----------------------
                //
                class Renderer_RTF : public Renderer
                {
                    private:
                        INT m_cCols;

                        LPSTR DoubleBackslashes(LPSTR pszText);

                        //
                        // Prevent copy.
                        //
                        Renderer_RTF(const Renderer_RTF& rhs);
                        Renderer_RTF& operator = (const Renderer_RTF& rhs);

                    protected:
                        virtual VOID Begin(INT cRows, INT cCols);

                        virtual VOID AddTitle(LPCWSTR pszTitleW);

                        virtual VOID BeginHeaders(VOID);

                        virtual VOID AddHeader(LPCWSTR pszHeaderW);

                        virtual VOID AddHeaderSep(VOID)
                            { AddRowColSep(); }

                        virtual VOID EndHeaders(VOID)
                            { m_Stm.Write("\\row "); }

                        virtual VOID BeginRow(VOID)
                            { BeginHeaderOrRow();
                              AddCellDefs(); }

                        virtual VOID AddRowColData(INT iRow, INT idCol);

                        virtual VOID AddRowColSep(VOID)
                            { m_Stm.Write("\\cell "); }

                        virtual VOID EndRow(VOID)
                            { m_Stm.Write("\\row "); }

                        virtual VOID End(VOID)
                            { m_Stm.Write(" \\pard \\widctlpar \\par }"); }

                        virtual VOID BeginHeaderOrRow(VOID);

                        virtual VOID AddCellDefs(VOID);


                    public:
                        Renderer_RTF(DetailsView& DV)
                            : Renderer(DV),
                              m_cCols(0) { }

                        virtual ~Renderer_RTF(VOID) { }
                };


                //
                // DetailsView::DataObject::Renderer_Export -------------------
                //
                class Renderer_Export : public Renderer
                {
                    private:
                        //
                        // Prevent copy.
                        //
                        Renderer_Export(const Renderer_Export& rhs);
                        Renderer_Export& operator = (const Renderer_Export& rhs);

                    protected:
                        virtual VOID Render(IStream *pStm);

                        virtual VOID Begin(INT cRows, INT cCols);

                        virtual VOID AddBinaryRecord(INT iRow);

                        virtual VOID End(VOID) { }

                    public:
                        Renderer_Export(DetailsView& DV)
                            : Renderer(DV) { }

                        virtual ~Renderer_Export(VOID) { }
                };

                //
                // DetailsView::DataObject::Renderer_FileGroupDescriptor ------
                //
                class Renderer_FileGroupDescriptor : public Renderer
                {
                    private:
                        //
                        // Prevent copy.
                        //
                        Renderer_FileGroupDescriptor(const Renderer_FileGroupDescriptor& rhs);
                        Renderer_FileGroupDescriptor& operator = (const Renderer_FileGroupDescriptor& rhs);

                    protected:
                        virtual VOID Begin(INT cRows, INT cCols);

                    public:
                        Renderer_FileGroupDescriptor(DetailsView& DV)
                            : Renderer(DV) { }

                        virtual ~Renderer_FileGroupDescriptor(VOID) { };
                };


                //
                // DetailsView::DataObject::Renderer_FileContents -------------
                //
                class Renderer_FileContents : public Renderer_Export
                {
                    private:
                        //
                        // Prevent copy.
                        //
                        Renderer_FileContents(const Renderer_FileContents& rhs);
                        Renderer_FileContents& operator = (const Renderer_FileContents& rhs);

                    protected:

                    public:
                        Renderer_FileContents(DetailsView& DV)
                            : Renderer_Export(DV) { }

                        virtual ~Renderer_FileContents(VOID) { };
                };

                //
                // DetailsView::DataObject private member variables.
                //
                IStorage    *m_pStg;                 // Storage pointer.
                IStream     *m_pStm;                 // Stream pointer.
                DetailsView& m_DV;

                //
                // Private functions to help with the rendering process.
                //
                HRESULT CreateRenderStream(DWORD tymed, IStream **ppStm);
                HRESULT RenderData(IStream *pStm, CLIPFORMAT cf);

                //
                // Prevent copy.
                //
                DataObject(const DataObject& rhs);
                DataObject& operator = (const DataObject& rhs);
        };

        //
        // DetailsView::DropSource --------------------------------------------
        //
        class DropSource
        {
            public:
                DropSource(DWORD grfKeyState)
                    : m_grfKeyState(grfKeyState) { }

                ~DropSource(VOID) { }
                DWORD m_grfKeyState;  // "Key" used to start drag/drop.

            private:

                //
                // Prevent copying.
                //
                DropSource(const DropSource&);
                void operator = (const DropSource&);
        };

        //
        // DetailsView::DropTarget --------------------------------------------
        //
        class DropTarget
        {
            public:
                DropTarget(DWORD grfKeyState)
                    : m_grfKeyState(grfKeyState),
                      m_pIDataObject(NULL) { }

                ~DropTarget(VOID) { };

                DWORD m_grfKeyState;  // "Key" used to start drag/drop.
                IDataObject *m_pIDataObject; // Ptr received through DragEnter.

            private:
                //
                // Prevent copying.
                //
                DropTarget(const DropTarget&);
                void operator = (const DropTarget&);
        };


        LONG               m_cRef;
        PointerList        m_UserList;         // List of user objects.
        HWND               m_hwndMain;         // Main window.
        HWND               m_hwndListView;     // Listview window.
        HWND               m_hwndStatusBar;    // Status bar.
        HWND               m_hwndToolBar;      // Tool bar.
        HWND               m_hwndToolbarCombo; // "Find User" combo box.
        HWND               m_hwndListViewToolTip;   // Tool tip window.
        HWND               m_hwndHeader;       // Listview header control.
        HACCEL             m_hKbdAccel;        // Accelerator table.
        WNDPROC            m_lpfnLVWndProc;    // We subclass the LV control.
        PDISKQUOTA_CONTROL m_pQuotaControl;    // Ptr to quota controller.
        Finder            *m_pUserFinder;      // For locating users in listview.
        UndoList          *m_pUndoList;        // For "undoing" mods and deletes.
        ColumnMap          m_ColMap;           // ColId to iSubItem map.
        DropSource         m_DropSource;
        DropTarget         m_DropTarget;
        DataObject        *m_pDataObject;
        CVolumeID          m_idVolume;
        CString            m_strVolumeDisplayName;
        CString            m_strAccountUnresolved;
        CString            m_strAccountUnavailable;
        CString            m_strAccountDeleted;
        CString            m_strAccountUnknown;
        CString            m_strAccountInvalid;
        CString            m_strNoLimit;
        CString            m_strNotApplicable;
        CString            m_strStatusOK;
        CString            m_strStatusWarning;
        CString            m_strStatusOverlimit;
        CString            m_strDispText;
        LPDATAOBJECT       m_pIDataObjectOnClipboard;
        POINT              m_ptMouse;          // For hit-testing tooltips.
        DWORD              m_dwEventCookie;    // Event sink cookie.
        INT                m_iLastItemHit;     // Last item mouse was over.
        INT                m_iLastColSorted;
        DWORD              m_fSortDirection;   // 0 = Ascending, 1 = Descending
        CRITICAL_SECTION   m_csAsyncUpdate;
        LV_STATE_INFO      m_lvsi;             // Persistent lv state info.
        BOOL               m_bMenuActive;      // Is a menu active?
        BOOL               m_bWaitCursor;      // Show wait cursor?
        BOOL               m_bStopLoadingObjects;
        BOOL               m_bDestroyingView;
        static const INT   MAX_FINDMRU_ENTRIES;
        static const INT   CX_TOOLBAR_COMBO;
        static const INT   CY_TOOLBAR_COMBO;


        HRESULT InitializeStaticStrings(VOID);
        HRESULT CreateMainWindow(VOID);
        HRESULT CreateListView(VOID);
        HRESULT CreateStatusBar(VOID);
        HRESULT CreateToolBar(VOID);
        HRESULT CreateListViewToolTip(VOID);
        HRESULT AddColumn(INT iColumn, const DV_COLDATA& ColDes);
        HRESULT RemoveColumn(INT iColumn);
        HRESULT AddImages(VOID);
        HRESULT LoadObjects(VOID);
        HRESULT ReleaseObjects(VOID);
        LRESULT SortObjects(DWORD idColumn, DWORD dwDirection);
        LRESULT Refresh(bool bInvalidateCache = false);
        LRESULT SelectAllItems(VOID);
        LRESULT InvertSelectedItems(VOID);
        LRESULT ShowItemCountInStatusBar(VOID);
        LRESULT ShowMenuTextInStatusBar(DWORD idMenuOption);
        VOID SaveViewStateToRegistry(VOID);
        VOID EnableMenuItem_ArrangeByFolder(BOOL bEnable);
        VOID EnableMenuItem_Undo(BOOL bEnable);
        VOID SetWaitCursor(VOID);
        VOID ClearWaitCursor(VOID);
        VOID Redraw(VOID)
            {
                RedrawWindow(m_hwndMain, NULL, NULL,
                             RDW_ERASE |
                             RDW_FRAME |
                             RDW_INVALIDATE |
                             RDW_ALLCHILDREN |
                             RDW_UPDATENOW);
            }

        VOID RedrawItems(VOID)
        {
            ListView_RedrawItems(m_hwndListView, -1, -1);
            UpdateWindow(m_hwndListView);
        }

        BOOL AddUser(PDISKQUOTA_USER pUser);
        INT  GetUserQuotaState(PDISKQUOTA_USER pUser);
        VOID RegisterAsDropTarget(BOOL bActive);
        bool SingleSelectionIsAdmin(void);

        //
        // Message handlers.
        //
        LRESULT OnNotify(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnSize(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnSetCursor(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnSetFocus(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnDestroy(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnMainWindowCreated(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnCommand(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnCmdViewStatusBar(VOID);
        LRESULT OnCmdViewToolBar(VOID);
        LRESULT OnCmdViewShowFolder(VOID);
        LRESULT OnCmdProperties(VOID);
        LRESULT OnCmdNew(VOID);
        LRESULT OnCmdDelete(VOID);
        LRESULT OnCmdUndo(VOID);
        LRESULT OnCmdFind(VOID);
        LRESULT OnCmdEditCopy(VOID);
        LRESULT OnCmdImport(VOID);
        LRESULT OnCmdExport(VOID);
        LRESULT OnMenuSelect(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnContextMenu(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnHelpAbout(HWND);
        LRESULT OnHelpTopics(HWND);
        LRESULT OnSettingChange(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnLVN_OwnerDataFindItem(NMLVFINDITEM *);
        LRESULT OnLVN_GetDispInfo(LV_DISPINFO *);
        LRESULT OnLVN_GetDispInfo_Text(LV_DISPINFO *, PDISKQUOTA_USER);
        LRESULT OnLVN_GetDispInfo_Image(LV_DISPINFO *, PDISKQUOTA_USER);
        LRESULT OnLVN_ColumnClick(NM_LISTVIEW *);
        LRESULT OnLVN_ItemChanged(NM_LISTVIEW *);
        LRESULT OnLVN_BeginDrag(NM_LISTVIEW *);
        LRESULT OnTTN_NeedText(TOOLTIPTEXT *);
        LRESULT LV_OnTTN_NeedText(TOOLTIPTEXT *);
        LRESULT LV_OnMouseMessages(HWND, UINT, WPARAM, LPARAM);
        BOOL HitTestHeader(int xPos, int yPos);

        INT_PTR ActivateListViewToolTip(BOOL bActivate)
            { return SendMessage(m_hwndListViewToolTip, TTM_ACTIVATE, (WPARAM)bActivate, 0); }

        VOID FocusOnSomething(VOID);
        VOID CleanupAfterAbnormalTermination(VOID);

        INT FindUserByName(LPCTSTR pszUserName, PDISKQUOTA_USER *ppIUser = NULL);
        INT FindUserBySid(LPBYTE pbUserSid, PDISKQUOTA_USER *ppIUser = NULL);
        INT FindUserByObjPtr(PDISKQUOTA_USER pIUser);
        BOOL GotoUserName(LPCTSTR pszUser);

        //
        // Connection point stuff.
        //
        HRESULT ConnectEventSink(VOID);
        HRESULT DisconnectEventSink(VOID);
        IConnectionPoint *GetConnectionPoint(VOID);

        static DWORD ThreadProc(DWORD);
        static INT CompareItems(LPVOID, LPVOID, LPARAM);
        static HRESULT CalcPctQuotaUsed(PDISKQUOTA_USER, LPDWORD);
        static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK LVSubClassWndProc(HWND, UINT, WPARAM, LPARAM);

        //
        // Prevent copying.
        //
        DetailsView(const DetailsView&);
        void operator = (const DetailsView&);


    public:
        DetailsView(VOID);
        ~DetailsView(VOID);

        BOOL Initialize(
            const CVolumeID& idVolume);

        CVolumeID GetVolumeID(void) const
            { return m_idVolume; }

        //
        // This is public so other UI elements can use it. (i.e. VolPropPage).
        //
        static HRESULT CreateVolumeDisplayName(
                const CVolumeID& idVolume, // [in] - "C:\" or "\\?\Volume{ <guid> }\"
                CString *pstrDisplayName); // [out] - "My Disk (C:)"
        //
        // If you change the value of idCol_Last, see the note with
        // the LV_STATE_INFO structure above regarding the rgcxCol[] member
        // of LV_STATE_INFO.
        //
        enum ColumnIDs { idCol_Status,
                         idCol_Folder,
                         idCol_Name,
                         idCol_LogonName,
                         idCol_AmtUsed,
                         idCol_Limit,
                         idCol_Threshold,
                         idCol_PctUsed,
                         idCol_Last };

        //
        // IUnknown methods.
        //
        STDMETHODIMP
        QueryInterface(
            REFIID riid,
            LPVOID *ppv);

        STDMETHODIMP_(ULONG)
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG)
        Release(
            VOID);

        //
        // IDiskQuotaEvents method.
        //
        STDMETHODIMP
        OnUserNameChanged(
            PDISKQUOTA_USER pUser);


        //
        // IDropSource methods.
        //
        STDMETHODIMP
        GiveFeedback(
            DWORD dwEffect);

        STDMETHODIMP
        QueryContinueDrag(
            BOOL fEscapePressed,
            DWORD grfKeyState);

        //
        // IDropTarget methods.
        //
        STDMETHODIMP DragEnter(
            IDataObject * pDataObject,
            DWORD grfKeyState,
            POINTL pt,
            DWORD * pdwEffect);

        STDMETHODIMP DragOver(
            DWORD grfKeyState,
            POINTL pt,
            DWORD * pdwEffect);

        STDMETHODIMP DragLeave(
            VOID);

        STDMETHODIMP Drop(
            IDataObject * pDataObject,
            DWORD grfKeyState,
            POINTL pt,
            DWORD * pdwEffect);


        //
        // IDataObject methods.
        //
        STDMETHODIMP
        GetData(
            FORMATETC *pFormatetc,
            STGMEDIUM *pmedium);

        STDMETHODIMP
        GetDataHere(
            FORMATETC *pFormatetc,
            STGMEDIUM *pmedium);

        STDMETHODIMP
        QueryGetData(
            FORMATETC *pFormatetc);

        STDMETHODIMP
        GetCanonicalFormatEtc(
            FORMATETC *pFormatetcIn,
            FORMATETC *pFormatetcOut);

        STDMETHODIMP
        SetData(
            FORMATETC *pFormatetc,
            STGMEDIUM *pmedium,
            BOOL fRelease);

        STDMETHODIMP
        EnumFormatEtc(
            DWORD dwDirection,
            IEnumFORMATETC **ppenumFormatetc);

        STDMETHODIMP
        DAdvise(
            FORMATETC *pFormatetc,
            DWORD advf,
            IAdviseSink *pAdvSink,
            DWORD *pdwConnection);

        STDMETHODIMP
        DUnadvise(
            DWORD dwConnection);

        STDMETHODIMP
        EnumDAdvise(
            IEnumSTATDATA **ppenumAdvise);



        HWND GetHWndMain(VOID)
            { return m_hwndMain; }

        static VOID InitLVStateInfo(PLV_STATE_INFO plvsi);
        static BOOL IsValidLVStateInfo(PLV_STATE_INFO plvsi);

        void GetVolumeDisplayName(CString *pstrName)
            { *pstrName = m_strVolumeDisplayName; }

        UINT GetColumnIds(INT *prgColIds, INT cColIds);

        //
        // Methods for getting drag-drop report data from details view.
        //
        INT GetNextSelectedItemIndex(INT iRow);
        BOOL GetReportItem(UINT iRow, UINT iColId, PLV_REPORT_ITEM pItem);
        VOID GetReportTitle(LPTSTR pszDest, UINT cchDest);
        VOID GetReportColHeader(UINT iColId, LPTSTR pszDest, UINT cchDest);
        UINT GetReportColCount(VOID);
        UINT GetReportRowCount(VOID);
        //
        // These methods are for generating binary "reports" used in exporting
        // user quota information for transfer between volumes.
        //
        UINT GetReportBinaryRecordSize(UINT iRow);
        BOOL GetReportBinaryRecord(UINT iRow, LPBYTE pbRecord, UINT cbRecord);

        //
        // NOTE:  If the requirement for friendship between DetailsView and
        //        DetailsView::Finder exceeds only a few instances, we
        //        might as well grant total friendship to the Finder class.
        //        As long as the instance count is small, I like to keep
        //        the friendship restricted as much as possible.
        //
        // This Finder::DlgProc needs to call DetailsView::GotoUserName.
        //
        friend BOOL Finder::UserNameEntered(HWND);
        //
        // Finder::DlgProc needs access to Details::CY_TOOLBAR_COMBO.
        //
        friend INT_PTR CALLBACK Finder::DlgProc(HWND, UINT, WPARAM, LPARAM);

        friend class Importer;
};



//
// Represents a selection in the listview.
// Objects of this type are used for communicating a selection set to
// a function.  The recpient of the LVSelection object can query it
// to obtain information about the selection.
//
class LVSelection
{
    private:
        HWND m_hwndListView;
        struct ListEntry
        {
            PDISKQUOTA_USER pUser;
            INT iItem;
        };

        StructureList m_List;

        //
        // Prevent copying.
        //
        LVSelection(const LVSelection&);
        void operator = (const LVSelection&);

    public:
        LVSelection(HWND hwndListView)
            : m_hwndListView(hwndListView),
              m_List(sizeof(ListEntry), 10) { }

        LVSelection(VOID)
            : m_hwndListView(NULL),
              m_List(sizeof(ListEntry), 1) { }

        ~LVSelection(VOID) { }

        VOID Add(PDISKQUOTA_USER pUser, INT iItem);
        HWND GetListViewHwnd(VOID)
            { return m_hwndListView; }
        INT Count(VOID)
            { return m_List.Count(); }
        BOOL Retrieve(INT i, PDISKQUOTA_USER *ppUser, INT *piItem);
        BOOL Retrieve(INT i, PDISKQUOTA_USER *ppUser)
            { return Retrieve(i, ppUser, NULL); }
        BOOL Retrieve(INT i, INT *pItem)
            { return Retrieve(i, NULL, pItem); }
        VOID Clear(VOID)
            { m_List.Clear(); }
};




#endif // _INC_DSKQUOTA_DETAILS_H


