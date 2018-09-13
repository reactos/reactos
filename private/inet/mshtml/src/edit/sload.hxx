//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  Class:      CSpringLoader
//
//  Contents:   CSpringLoader Class
//
//              A CSpringLoader - intercepts and applies character formatting
//              commands.
//
//              It is tied to the caret object
//
//
//  History:    07/13/98  OliverSe Created
//----------------------------------------------------------------------------

#ifndef _SLOAD_HXX_
#define _SLOAD_HXX_ 1

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

class CCommand;
class CBaseCharCommand;
class CCharCommand;
class CFontCommand;
class CMshtmlEd;

MtExtern(CSpringLoader)

enum SPRINGLOADER_MODE;

class CSpringLoader
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CSpringLoader))
    CSpringLoader(CMshtmlEd * pCommandTarget);
    CSpringLoader() { Assert(!"Can't use default constructor"); }
    ~CSpringLoader();

    //
    // Springloading interface.
    //

    HRESULT SpringLoad(IMarkupPointer * pmpPosition, DWORD dwMode = 0);
    HRESULT SpringLoadComposeSettings(IMarkupPointer * pmpNewPosition, BOOL fReset = FALSE, BOOL fOutsideSpan = FALSE);
    HRESULT CanSpringLoadComposeSettings(IMarkupPointer * pmpNewPosition, BOOL * pfCanOverrideSpringLoad = NULL, BOOL fOutsideSpan = FALSE, BOOL fDontJump = FALSE);
    HRESULT Fire(IMarkupPointer * pmpStart, IMarkupPointer * pmpEnd = NULL, BOOL fMoveCaretToStart = TRUE);
    void    Reset(IMarkupPointer * pmpPosition = NULL);
    BOOL    IsSpringLoadedAt(IMarkupPointer * pmpPosition);
    BOOL    IsSpringLoaded()                                       { return _fSpringLoaded; }
    void    Reposition(IMarkupPointer * pmpPosition)               { if (_fSpringLoaded) MarkSpringLoaded(pmpPosition); }
    BOOL    OverrideComposeSettings()                              { return _fOverrideComposeSettings; }
    void    OverrideComposeSettings(BOOL fOverrideComposeSettings) { _fOverrideComposeSettings = fOverrideComposeSettings; }
    
    //
    // Exec and QueryStatus hooks.
    //

    HRESULT PrivateExec(DWORD nCmdID, VARIANTARG * pvarargIn, VARIANTARG * pvarargOut, ISegmentList * pSegmentList);
    HRESULT PrivateQueryStatus(DWORD nCmdID, OLECMD rgCmds[]);

private:

    //
    // General springloader helpers.
    //

    void    SetFormats(BOOL fBold, BOOL fItalic, BOOL fUnderline, BOOL fSuperscript, BOOL fSubscript, CVariant * pvarName, CVariant * pvarSize, CVariant * pvarColor, CVariant * pvarBackColor);
    void    MarkSpringLoaded(IMarkupPointer * pmpPosition);
    HRESULT EnsureCaretPointers(IMarkupPointer * pmpPosition, IHTMLCaret ** ppCaret, IMarkupPointer ** ppmpCaret );
    HRESULT UpdatePointerPositions(IMarkupPointer * pmpStart, IMarkupPointer * pmpEnd, ELEMENT_TAG_ID tagIdScope, BOOL fApplyElement);
    HRESULT CanHandleCommand(DWORD nCmdID, ISegmentList * pSegmentList, IMarkupPointer ** ppmpPosition=NULL, VARIANTARG * pvarargIn=NULL);
    HRESULT AdjustPointerForInsert(IMarkupPointer * pWhereIThinkIAm, BOOL fNotAtBOL, INT inBlockcDir, INT inTextDir);

    //
    // Compose settings helpers.
    //

    BOOL    IsBlockEmptyForSpringLoading(IMarkupPointer * pmpNewPosition, IHTMLElement * pElemBlock, BOOL fDontJump);
    BOOL    SpringLoadFormatsAcrossLayout(IMarkupPointer * pmpNewPosition, IHTMLElement * pElemBlock, BOOL fActuallySpringLoad = TRUE);
    BOOL    IsCharFormatBlock(IHTMLElement * pElem);
    BOOL    CanJumpOverElement(IHTMLElement * pElem);
    BOOL    InSpanScope(IMarkupPointer * pmpPosition);
    BOOL    IsSeparatedOnlyByPhraseScopes(IMarkupPointer * pmpLeft, IMarkupPointer * pmpRight);
    BOOL    IsDocumentInHTMLEditMode();

    //
    // Accessors.
    //

    IMarkupServices   * GetMarkupServices();
    IHTMLViewServices * GetViewServices();
    CCommand          * GetCommand(DWORD cmdID);
    CBaseCharCommand  * GetBaseCharCommand(DWORD cmdID);
    CCharCommand      * GetCharCommand(DWORD cmdID);
    CFontCommand      * GetFontCommand(DWORD cmdID);

    
    //
    // Data members.
    //

#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union
    {
        DWORD             _grfFlagsVar;
        struct
        {
#else
            DWORD         _Padding;
#endif
            DWORD         _fSpringLoaded:1;
            DWORD         _fBold:1;
            DWORD         _fItalic:1;
            DWORD         _fUnderline:1;
            DWORD         _fSuperscript:1;
            DWORD         _fSubscript:1;
            DWORD         _fOverrideComposeSettings:1;
            DWORD         _fUnused:24;
#if !defined(MW_MSCOMPATIBLE_STRUCT)
        };
    };

    DWORD& _grfFlags() { return _grfFlagsVar; }
#else
    DWORD& _grfFlags() { return *(((DWORD*)&_Padding) +1); }
#endif

    CVariant              _varName;
    CVariant              _varSize;
    CVariant              _varColor;
    CVariant              _varBackColor;
    CVariant              _varSpanClass;

    CMshtmlEd           * _pCommandTarget;   // Pointer to command target owning this springloader
    IMarkupPointer      * _pmpPosition;      // Position springloader at which springloader was loaded.
};


enum SPRINGLOADER_MODE
{
    SL_DEFAULT                 = 0,  // Simply springload at position specified
    SL_ADJUST_FOR_INSERT_LEFT  = 1,  // Adjust pointer for insert left before springloading
    SL_ADJUST_FOR_INSERT_RIGHT = 2,  // Adjust pointer for insert right
    SL_TRY_COMPOSE_SETTINGS    = 4,  // Springload compose settings, if possible at position
    SL_RESET                   = 8,  // Reset the springloader prior to loading
};

#endif //_SLOAD_HXX_
