#ifndef I_EVNTPRM_HXX_
#define I_EVNTPRM_HXX_
#pragma INCMSG("--- Beg 'evntprm.hxx'")

class CTreeNode;
class CDoc;
class CVariant;
class CEventObj;
class CPersistData;
class CDataSourceProvider;
interface ICSSFilter;
interface INamedPropertyBag;


//----------------------------------------------------------------------------------
// Typedefs for custom parameters    
//----------------------------------------------------------------------------------
struct SErrorParams
{
    LPCTSTR                 pchErrorMessage;    // error message    
    LPCTSTR                 pchErrorUrl;        // document in which the error occured    
    long                    lErrorLine;         // line number    
    long                    lErrorCharacter;    // character offset    
    long                    lErrorCode;         // error code    
};

struct SMessageParams
{
    LPCTSTR                 pchMessageText;             // text for message dialog    
    LPCTSTR                 pchMessageCaption;          // title for message dialog    
    DWORD                   dwMessageStyle;             // style of message dialog    
    LPCTSTR                 pchMessageHelpFile;         // help file for message dialog    
    DWORD                   dwMessageHelpContext;       // context for message dialog help    
};                       

struct SPagesetupParams
{
    LPCTSTR                 pchPagesetupHeader;         // header string    
    LPCTSTR                 pchPagesetupFooter;         // footer string    
    LONG_PTR                lPagesetupDlg;              // address of PageSetupDlgW struct        
};
    
struct SPrintParams
{    
    unsigned int            fPrintRootDocumentHasFrameset:1;    // print flags    
    unsigned int            fPrintAreRatingsEnabled:1;          // print flags    
    unsigned int            fPrintActiveFrame:1;                // print flags    
    unsigned int            fPrintLinked:1;                     // print flags    
    unsigned int            fPrintSelection:1;                  // print flags    
    unsigned int            fPrintAsShown:1;                    // print flags    
    unsigned int            fPrintShortcutTable:1;              // print flags    
    unsigned int            fPrintToFileOk:1;                   // PrintToFile dialog success flag    
    int                     iPrintFontScaling;                      
    IOleCommandTarget *     pPrintBodyActiveTarget;             // for Layout()->LockFocusRect()    
    LONG_PTR                lPrintDlg;                          // address of PrintDlgW struct            
    LPCTSTR                 pchPrintToFileName;                 // PrintToFile file name   
#ifdef UNIX    
    int                     iPrintdmOrientation;                // device orienatation
#endif // UNIX    
};

struct SPropertysheetParams
{
    SAFEARRAY   * paPropertysheetPunks;           // array of punks to display property sheets for
};


MtExtern(EVENTPARAM)
struct EVENTPARAM 
{
    friend class CEventObj;

    DECLARE_MEMALLOC_NEW_DELETE(Mt(EVENTPARAM))

    EVENTPARAM(CDoc * pDoc, BOOL fInitState, BOOL fPush = TRUE);
    ~EVENTPARAM();
    
    void Push();
    void Pop();

    BOOL IsCancelled();

    CTreeNode *             _pNode;             // src element
    CTreeNode *             _pNodeFrom;         // for move,over,out
    CTreeNode *             _pNodeTo;           // for move,over,out 
    short                   _sKeyState;         // Current key state 
                                                //   VB_ALT, etc.
    long                    _lKeyCode;          // keycode (keyup,down,press)
    long                    _lButton;           // mouse button if applicable
    long                    _screenX;           // xpos relative to UL of user's screen
    long                    _screenY;           // ypos
    long                    _clientX;           // xpos relative to UL of client window
    long                    _clientY;           // ypos

    long                    _offsetX;
    long                    _offsetY;
    long                    _x;
    long                    _y;
    
    CVariant                varReturnValue;     // variant-flag mainly for cancel the default action 
                                                // (can be only VT_EMPTY or VT_BOOL;
                                                // when VT_BOOL, VB_FALSE iff cancel), but also used 
                                                // for returning strings
    BOOL                    fCancelBubble;      // Flag on whether to stop
                                                //   bubbling this event.
    DWORD                   dwDropEffect;       // for ondragenter, ondragover    
    EVENTPARAM *            pparamPrev;         // Prev event param object
    CDoc *                  pDoc;               // The containing document
    long                    _lReason;           // Reason argument for dataSetComplete event.
    long                    lSubDivision;       //  the area index for an image
    BOOL                    fRepeatCode;        // repeat count for keydown events
    CDataSourceProvider *   pProvider;          // provider (for data events)
    ICSSFilter *			psrcFilter;		    // source filter

    CEventObj *             pEventObj;          // normally NULL, points to CEventObject in case
                                                // when created on heap
    
    // Custon parameters
    union 
    {
        SErrorParams                 errorParams;
        SMessageParams               messageParams;
        SPagesetupParams             pagesetupParams;
        SPrintParams                 printParams;
        SPropertysheetParams         propertysheetParams;
    };

    inline void SetPropName(LPCTSTR pchName) { _cstrPropName.Set(pchName); }
    inline LPCTSTR GetPropName() { return _cstrPropName;}

    inline void SetType(LPCTSTR pchName) { _cstrType.Set(pchName); }
    inline LPCTSTR GetType() { return _cstrType;}

    inline void SetQualifier(LPCTSTR pchName) { _cstrQualifier.Set(pchName); }
    inline LPCTSTR GetQualifier() { return _cstrQualifier;}

    inline void SetSrcUrn(LPCTSTR pchName) { _cstrSrcUrn.Set(pchName); }
    inline LPCTSTR GetSrcUrn()             { return _cstrSrcUrn; }

    void SetNodeAndCalcCoordinates(CTreeNode *pNewNode);

    // helper functions
    HRESULT GetParentCoordinates(long *px, long *py);

private:
    HRESULT     CalcRestOfCoordinates();
    CStr                    _cstrType;          // string for event type
    CStr                    _cstrPropName;      // string for property name for onpropertyChange
    CStr                    _cstrQualifier;     // Qualifier for dataSet events
    CStr                    _cstrSrcUrn;        // urn of source (behavior)

    BOOL                    _fOnStack : 1;      // TRUE while the EVENTPARAM is on stack.
                                                // This bit is used to avoid doing POP twice (one time to balance
                                                // explicit PUSH, and one more time from destructor)
};

#pragma INCMSG("--- End 'evntprm.hxx'")
#else
#pragma INCMSG("*** Dup 'evntprm.hxx'")
#endif
