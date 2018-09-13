//
//  MSG.HXX
//
//  Base form class definition
//
//  Copyright (c) 1986-1996, Microsoft Corporation.
//  All rights reserved.
//


#include        "padrc.h"
#include        "chsfld.h"
#include        "pad.hxx"
#include        "msgerr.hxx"
#include        "mshtmhst.h"
#include        "richedit.h"

// Forward Declarations

class CPadMessage;

//
//  Mapi properties
//

#define PR_HTML_BODY  PROP_TAG( PT_STRING8, 0x6800 )

//
//  Mapi recipient types
//

#define MAPI_TYPE_COUNT 4

//
//  Form states
//

enum { stateUninit, stateNormal, stateNoScribble, stateHandsOffFromNormal,
        stateHandsOffFromSave, stateDead };

//
//  Message properties held in memory
//

enum {
    irtTo,
    irtCc,
    irtSubject,
    irtTime,
    irtClass,
    irtNormSubject,
    irtConvIdx,
    irtConvTopic,
    irtSenderName,

    cPropSendMsg,

    irtHtmlBody = cPropSendMsg,
    irtSenderAddrType,
    irtSenderEntryid,
    irtSenderEmailAddress,
    irtSenderSearchKey,
    irtReceivedBySearchKey,
    irtReplyRecipientEntries,
    irtReplyRecipientNames,

    cPropReadMsg
};

#define MESSAGE_TAGS            \
    PR_DISPLAY_TO_A,            \
    PR_DISPLAY_CC_A,            \
    PR_SUBJECT_A,               \
    PR_CLIENT_SUBMIT_TIME,      \
    PR_MESSAGE_CLASS_A,         \
    PR_NORMALIZED_SUBJECT_A,    \
    PR_CONVERSATION_INDEX,      \
    PR_CONVERSATION_TOPIC_A,    \
    PR_SENDER_NAME_A,           \
    PR_HTML_BODY,               \
    PR_SENDER_ADDRTYPE,         \
    PR_SENDER_ENTRYID,          \
    PR_SENDER_EMAIL_ADDRESS_A,  \
    PR_SENDER_SEARCH_KEY,       \
    PR_RECEIVED_BY_SEARCH_KEY,  \
    PR_REPLY_RECIPIENT_ENTRIES, \
    PR_REPLY_RECIPIENT_NAMES_A

//form type
enum { eformRead, eformSend};

//reply type  (this form does not implement reply all)
enum eREPLYTYPE {eREPLY, eFORWARD, eREPLY_ALL};

//
// View notification sink
//

#define MAXSINKS    32

class CViewNotifier
{
private:
    LPMAPIVIEWADVISESINK    m_aryAdviseSink [MAXSINKS];

public:

    CViewNotifier  (void);
    ~CViewNotifier ();

    BOOL        Initialize (void);

    HRESULT     Advise (LPMAPIVIEWADVISESINK pAdvise, ULONG * pulConnection);
    HRESULT     Unadvise (ULONG ulConnection);

    void        OnShutdown (void);
    void        OnNewMessage (void);
    HRESULT     OnPrint (ULONG ulPageNumber, HRESULT hrStatus);
    void        OnSubmitted (void);
    void        OnSaved (void);
};


//
// Trident Exchange form
//


class CPadMessageDocHost :  public IDocHostUIHandler,
                            public IDocHostShowUI
{
public:

    DECLARE_SUBOBJECT_IUNKNOWN(CPadMessage, PadMessage);

    // IDocHostUIHandler
    STDMETHOD(GetHostInfo) (DOCHOSTUIINFO * pInfo);
    STDMETHOD(ShowUI) (
            DWORD dwID,
            IOleInPlaceActiveObject * pActiveObject,
            IOleCommandTarget * pCommandTarget,
            IOleInPlaceFrame * pFrame,
            IOleInPlaceUIWindow * pDoc);
    STDMETHOD(HideUI) (void);
    STDMETHOD(UpdateUI) (void);
    STDMETHOD(EnableModeless)(BOOL fEnable);
    STDMETHOD(OnDocWindowActivate) (BOOL fActivate);
    STDMETHOD(OnFrameWindowActivate) (BOOL fActivate);
    STDMETHOD(ResizeBorder) (
            LPCRECT prcBorder,
            IOleInPlaceUIWindow * pUIWindow,
            BOOL fRameWindow);
    STDMETHOD(ShowContextMenu) (
            DWORD dwID,
            POINT * pptPosition,
            IUnknown * pcmdtReserved,
            IDispatch * pDispatchObjectHit);
    STDMETHOD(TranslateAccelerator) (
            LPMSG lpMsg,
            const GUID * pguidCmdGroup, 
            DWORD nCmdID);
    STDMETHOD(GetOptionKeyPath) (BSTR *pbstrKey, DWORD dw);

    // IDocHostShowUI
    STDMETHOD(ShowMessage) (
            HWND     hwnd,
            LPOLESTR lpstrText,
            LPOLESTR lpstrCaption,
            DWORD dwType,
            LPOLESTR lpstrHelpFile,
            DWORD dwHelpContext,
            LRESULT * plResult);
    STDMETHOD(ShowHelp) (
            HWND hwnd,
            LPOLESTR pszHelpFile,
            UINT uCommand,
            DWORD dwData,
            POINT ptMouse,
            IDispatch * pDispatchObjectHit);
    STDMETHOD(GetDropTarget) (
            IDropTarget * pDropTarget, 
            IDropTarget ** ppDropTarget);
    STDMETHOD(GetExternal) (IDispatch **ppDisp); 
    STDMETHOD(TranslateUrl) (DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
    STDMETHOD(FilterDataObject) (IDataObject *pDO, IDataObject **ppDORet);
};


class CPadMessage : public CPadDoc,
                    public IPersistMessage,
                    public IMAPIForm,
                    public IMAPIFormAdviseSink
{
    friend class CPadMessageDocHost;
    ULONG               _cRef;                 // Reference Count on object
    ULONG               _state;                // uses state enum
    CLastError          _lsterr;               // Last Error Implementation


    CViewNotifier       _viewnotify;

    LPMAPIVIEWCONTEXT   _pviewctxOverride;
    LPMAPIVIEWCONTEXT   _pviewctx;             // View context interface

    LPMESSAGE           _pmsg;                 // our message
    LPMAPIMESSAGESITE   _pmsgsite;             // our message site
    LPMAPISESSION       _pses;                 // our MAPI session
    LPADRBOOK           _pab;                  // our address book

    LPADRLIST           _padrlist;             // Current recipient list
    LPSPropValue        _pval;                 // Current message contents

    ULONG               _ulMsgFlags;           // Message flags
    ULONG               _ulMsgStatus;          // Message status flags
    ULONG               _ulSiteStatus;         // Message Site status flags
    ULONG               _ulViewStatus;         // View context status flags

    ULONG               _cxMin;                // Minimium size of window
    ULONG               _cyMin;                //

    int                 _eFormType;            //read/write
    unsigned int        _fSameAsLoaded:1;      // Copy of ::Save flag
    unsigned int        _fRecipientsDirty:1;   // Is the recip list dirty?
    unsigned int        _fDirty:1;
    unsigned int        _fConvTopicSet:1;
    unsigned int        _fExchangeMsg:1;        // we are an Exchange message form
    unsigned int        _fNewMessage:1;

    unsigned int        _fShowUI:1;

    ULONG               _cbConvIdx;
    LPBYTE              _lpbConvIdx;

    HACCEL              _HAccelTable;

    HMODULE             _hChsFldDll;
    HRPICKFOLDER        _lpfnHrPickFolder;
    ULONG               _cbCFDState;
    LPBYTE              _pbCFDState;

    friend class CTripCall;

public:

    HWND                _hwndDialog;
    RECT                _rcDialog;
    //Embedded Objects
    CPadMessageDocHost  _DocHost;

public:

    CPadMessage();
    virtual ~CPadMessage(void);


    MAPI_GETLASTERROR_METHOD(IMPL);
    MAPI_IPERSISTMESSAGE_METHODS(IMPL);
    MAPI_IMAPIFORM_METHODS(IMPL);
    MAPI_IMAPIFORMADVISESINK_METHODS(IMPL);

    // IUnknown methods

    STDMETHOD_(ULONG, AddRef)()     {return CPadDoc::AddRef();}
    STDMETHOD_(ULONG, Release)()    {return CPadDoc::Release();}
    STDMETHOD(QueryInterface)(REFIID, void **);

    //  IServiceProvider methods
    HRESULT QueryService(REFGUID sid, REFIID iid, void ** ppv);


    // Utility methods

    LPMESSAGE           Message(void) { return _pmsg; }
    LPMAPIMESSAGESITE   MsgSite(void) { return _pmsgsite; }
    LPMAPIVIEWCONTEXT   ViewCtx(void) { return (_pviewctxOverride ? _pviewctxOverride:_pviewctx); }
    void                Address(int);

    // Commands

    void                DoSubmit(void);
    void                DoDelete(void);
    void                DoCopy(void);
    void                DoMove(void);
    void                DoNext(ULONG ulDir);
    void                DoReply(eREPLYTYPE);
    void                DoCheckNames();

    // Field dialog management

    BOOL    DlgProcSend(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL    DlgProcRead(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void    OnDialogControlActivate(HWND hwndControl);
    void    HookControl(ULONG ulIdControl);
    HWND    CreateField(
                ULONG ulIdField,
                ULONG ulIdLabel,
                DWORD dwStyle,
                BOOL fCreateOleCallBack);
    HDWP    FieldRequestResize (
                HDWP    hdwp,
                RECT *  prcResize,
                ULONG   ulIdResize,
                ULONG   ulIdField,
                ULONG   ulIdLabel,
                RECT *  prcSubmitBtn,
                int *   pcyDialogHeight);

private:
    // CPadDoc methods

    virtual BOOL        OnTranslateAccelerator(MSG *pMsg);
    virtual void        Passivate();
    virtual HRESULT     DoSave(BOOL fPrompt);
    virtual LRESULT     PadWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
    virtual void        GetViewRect(RECT *prc, BOOL fIncludeObjectAdornments);
    virtual HRESULT     InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS pmgw);
    virtual void        SetDocTitle(TCHAR * pchTitle);
    virtual LRESULT     OnSize(WORD fwSizeType, WORD nWidth, WORD nHeight);

    // Message persistence

    HRESULT             DisplayMessage(void);
    HRESULT             GetMsgDataFromMsg(LPMESSAGE pmsg, ULONG ulMsgFlags);
    HRESULT             GetHtmlBodyFromMsg(char ** ppchBuffer);
    HRESULT             GetSubjectFromUI();
    BOOL                IsAddressed(void);
    HRESULT             SaveInto(LPMESSAGE);
    void                ClearDirty(void);
    BOOL                GetDirtyState();

    HRESULT             StreamInHtmlBody(LPMESSAGE pmsg);
    HRESULT             StreamInHtmlBody(char * pch);
    HRESULT             StreamOutHtmlBody(LPMESSAGE pmsg);
    HRESULT             StreamOutTextBody(LPMESSAGE pmsg);

    // Address book

    HRESULT             OpenAddrBook();

    // Recipients manipulation

    BOOL                AreWellsDirty();
    HRESULT             GetAndCheckRecipients(BOOL fUpdateWells);
    HRESULT             ParseRecipients(BOOL fIncludeFrom);
    HRESULT             DisplayRecipients(BOOL fIncludeFrom);
    HRESULT             AddRecipientsToWells(LPADRLIST pal);
    HRESULT             AddRecipientToWell(HWND hwndEdit, LPADRENTRY pae, BOOL fAddSemi, BOOL fCopyRow);
    HRESULT             AddNamesToAdrlist(LPADRLIST * ppal);
    HRESULT             BuildSelectionAdrlist(LPADRLIST * ppal, HWND hwndEdit, CHARRANGE * pchrg);

    // Reply - Forward

    HRESULT             Reply(eREPLYTYPE eType, HWND hwndParent, LPCRECT rcPos);
    HRESULT             ComposeReply(IStream *);
    HRESULT             SaveAsString(char ** ppchBuffer);
    HRESULT             GetReplieeAdrEntry(LPADRENTRY pae);
    HRESULT             SetReplieeEntryId(LPADRLIST * ppal, ULONG cbEid, LPENTRYID peid,
                            LPSTR szName, ULONG * piRepliee);
    HRESULT             ZapIfMatch(LPADRENTRY pae, BOOL fSearchKey,
                            ULONG cbFrom, LPBYTE pbFrom, BOOL fZap, LPBOOL pfMatch);
    HRESULT             SetReplyForwardRecipients(LPMESSAGE pmsgResend, eREPLYTYPE eReplyType);

    // User interface helpers

    HRESULT             OpenForm(HWND, LPCRECT, ULONG);
    HRESULT             InitReadToolbar ();
    int                 ShowMessageBox(HWND, LPCTSTR, LPCTSTR, UINT);
    void                ShowError();
    void                ConfigMenu(HMENU hMenu);
    inline void         ConfigWinMenu(void);

    // Helper functions for host integration

    virtual LRESULT     OnCommand(WORD wNotifyCode, WORD idm, HWND hwndCtl);
    virtual LRESULT     OnInitMenuPopup(HMENU hmenuPopup, UINT uPos, BOOL fSystemMenu);

    HRESULT             CreateToolbarUI();
    void                LoadToolbarUI();
    LRESULT             UpdateToolbarUI();

    HRESULT             GetContextMenu(HMENU *phmenu, int id);

private:

    // Data Members

/*    ULONG               _cRef;                 // Reference Count on object
    ULONG               _state;                // uses state enum

    CViewNotifier       _viewnotify;

    LPMAPIVIEWCONTEXT   _pviewctxOverride;
    LPMAPIVIEWCONTEXT   _pviewctx;             // View context interface

    LPMESSAGE           _pmsg;                 // our message
    LPMAPIMESSAGESITE   _pmsgsite;             // our message site
    LPMAPISESSION       _pses;                 // our MAPI session
    LPADRBOOK           _pab;                  // our address book

    LPADRLIST           _padrlist;             // Current recipient list
    LPSPropValue        _pval;                 // Current message contents

    ULONG               _ulMsgFlags;           // Message flags
    ULONG               _ulMsgStatus;          // Message status flags
    ULONG               _ulSiteStatus;         // Message Site status flags
    ULONG               _ulViewStatus;         // View context status flags

    int                 _eFormType;            //read/write

    unsigned int        _fSameAsLoaded:1;      // Copy of ::Save flag
    unsigned int        _fRecipientsDirty:1;   // Is the recip list dirty?
    unsigned int        _fDirty:1;
    unsigned int        _fConvTopicSet:1;
    unsigned int        _fExchangeMsg:1;        // we are an Exchange message form
    unsigned int        _fNewMessage:1;
    unsigned int        _fShowUI:1;

    ULONG               _cbConvIdx;
    LPBYTE              _lpbConvIdx;

    HACCEL              _HAccelTable;

    HMODULE             _hChsFldDll;
    HRPICKFOLDER        _lpfnHrPickFolder;
    ULONG               _cbCFDState;
    LPBYTE              _pbCFDState;
*/
    // Recipient wells

    ULONG               _cRecipTypes;
    ULONG               _rgulRecipTypes[MAPI_TYPE_COUNT];
    HWND                _rghwndEdit[MAPI_TYPE_COUNT];

    // Context menus

    HMENU               _hMenuCtx;

    // Toolbar

    HWND                _hwndTBFormat;

    // Standard TB Combobox Controls

    // Format TB Combobox Controls

    HWND                _hwndComboTag;
    HWND                _hwndComboFont;
    HWND                _hwndComboSize;
    HWND                _hwndComboColor;
};


//
//     CPadMessage::ConfigWinMenu()
//
inline void CPadMessage::ConfigWinMenu()
{
    if(NULL != _hwnd)
    {
        HMENU hmenu = GetMenu(_hwnd);
        if(NULL != hmenu)
            ConfigMenu(hmenu);
    }
}


//
// Mail utility functionsd
//

// MAPI Rows manipulation

HRESULT CopyRow(LPVOID pv, LPSRow prwSrc, LPSRow prwDst);
VOID    FreeSRowSet(LPSRowSet prws);

// Address list management

HRESULT GetMsgAdrlist (LPMESSAGE pmsg, LPSRowSet *  ppAdrList, CLastError *);
HRESULT AddRecipientToAdrlist(LPADRLIST * ppal, LPADRENTRY pae, ULONG *pulIndex);

// Property value manipulation

#define imodeCopy       0
#define imodeFlatten    1
#define imodeUnflatten  2

ULONG   CbForPropValue(LPSPropValue pval);
LPSPropValue    PvalFind(LPSRow prw, ULONG ulPropTag);
LPBYTE  CopyPval(LPSPropValue pvalSrc, LPSPropValue pvalDst,
               LPBYTE pb, LPBYTE * ppbDest, UINT imode);

// Misc helper functions

void    FormatTime(FILETIME *pft, LPSTR szTime, DWORD cchTime);

// Misc macros

#define PAD4(x)         (((x)+3)&~3)


//
// Global variables
//

extern LPVOID   g_lpCtl3d;
extern TCHAR    g_achFormName[];
extern TCHAR    g_achWindowCaption[];
extern char     g_achFormClassName[];

extern CPadFactory Factory;
extern CLastError * g_pLastError;
#define g_LastError (*g_pLastError)
