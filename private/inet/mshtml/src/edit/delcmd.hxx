//+------------------------------------------------------------------------
//
//  File:       DelCmd.hxx
//
//  Contents:   CDeleteCommand Class.
//
//  History:    07-14-98 ashrafm - moved from edcom.hxx
//
//-------------------------------------------------------------------------

#ifndef _DELCMD_HXX_
#define _DELCMD_HXX_ 1

//
// MtExtern's
//
class CHTMLEditor;
class CSpringLoader;
    
MtExtern(CDeleteCommand)

//+---------------------------------------------------------------------------
//
//  CDeleteCommand Class
//
//----------------------------------------------------------------------------

class CDeleteCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDeleteCommand))

    CDeleteCommand(DWORD cmdId, CHTMLEditor * pEd )
    :  CCommand( cmdId, pEd )
    {
    }

    virtual ~CDeleteCommand()
    {
    }

    HRESULT AdjustPointersForDeletion( IMarkupPointer* pStart, IMarkupPointer* pEnd );
    
    HRESULT Delete( IMarkupPointer* pStart, IMarkupPointer* pEnd, BOOL fAdjustPointers = FALSE );   
    
    BOOL IsValidOnControl();

    HRESULT DeleteCharacter( IMarkupPointer* pPointer, 
                             BOOL fLeftBound, 
                             BOOL fWordMode,
                             IMarkupPointer* pBoundry );

    HRESULT LaunderSpaces ( IMarkupPointer  * pStart,
                             IMarkupPointer  * pEnd );

protected:
    
    HRESULT PrivateExec( 
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pcmd,
                         OLECMDTEXT * pcmdtext );


private:

    HRESULT RemoveBlockIfNecessary( IMarkupPointer * pStart, IMarkupPointer * pEnd );

    HRESULT RemoveEmptyListContainers( IMarkupPointer * pStart );

    HRESULT AdjustOutOfBlock( IMarkupPointer * pStart, BOOL * pfAdjusted );

    HRESULT MergeDeletion( IMarkupPointer* pStart, IMarkupPointer* pEnd, BOOL fAdjustPointers );

    BOOL    HasLayoutOrIsBlock( IHTMLElement * pIElement );


    BOOL    IsMergeNeeded( IMarkupPointer * pStart, IMarkupPointer * pEnd );

    HRESULT InflateBlock( IMarkupPointer  * pStart );

    BOOL    IsLaunderChar ( TCHAR ch );

    BOOL    IsInPre( IMarkupPointer  * pStart,
                     IHTMLElement   ** ppPreElement );

    BOOL    IsIntrinsicControl( IHTMLElement * pHTMLElement );

    HRESULT ReplacePrevChar ( TCHAR ch,
                              IMarkupPointer  * pCurrent,
                              IMarkupServices * pMarkupServices );

    HRESULT SkipBlanks( IMarkupPointer * pPointerToMove,
                        Direction        eDir,
                        long           * pcch );


};


#endif

