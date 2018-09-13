//+------------------------------------------------------------------------
//
//  File:       InsCmd.hxx
//
//  Contents:   CInsertCommand Class.
//
//  History:    07-14-98 ashrafm - moved from edcom.hxx
//              08-14-98 raminh  - added InsertObjectCommand
//-------------------------------------------------------------------------

#ifndef _INSCMD_HXX_
#define _INSCMD_HXX_ 1

class CHTMLEditor;

//
// MtExtern's
//
    
MtExtern(CInsertCommand)
MtExtern(CInsertObjectCommand)
MtExtern(CInsertParagraphCommand)

extern "C" const CLSID CLSID_HTMLImg;
extern "C" const CLSID CLSID_HTMLButtonElement;
//extern "C" const CLSID CLSID_HTMLInputTextElement;
extern "C" const CLSID CLSID_HTMLTextAreaElement;
//extern "C" const CLSID CLSID_HTMLOptionButtonElement;
extern "C" const CLSID CLSID_HTMLListElement;
extern "C" const CLSID CLSID_HTMLMarqueeElement;
extern "C" const CLSID CLSID_HTMLDivPosition;
extern "C" const CLSID CLSID_HTMLHRElement;
extern "C" const CLSID CLSID_HTMLIFrame;
//extern "C" const CLSID CLSID_HTMLInputButtonElement;
//extern "C" const CLSID CLSID_HTMLInputFileElement;
extern "C" const CLSID CLSID_HTMLFieldSetElement;
extern "C" const CLSID CLSID_HTMLParaElement;
extern "C" const CLSID CLSID_HTMLInputElement;

//
// s_aryIntrinsicsClsid[] table is used to map Forms3 controls
// to their corresponding HTML tags, denoted by the CmdId field. 
// This table enables us to handle the scenario where user picks
// a Forms3 control using the Insert Object Dialog. In this case
// rather than inserting an <OBJECT> with the specified class id,
// we instantiate the corresponding HTML tag.
//

static const struct HTMLSTRMAPPING
{
    DWORD           CmdId;
    const CLSID   * pClsId;
}
s_aryIntrinsicsClsid[] =
{
    { IDM_IMAGE,            &CLSID_HTMLImg                  },                
    { IDM_BUTTON,           &CLSID_HTMLButtonElement        },      
    { IDM_TEXTBOX,          &CLSID_HTMLInputElement         },   
    { IDM_TEXTAREA,         &CLSID_HTMLTextAreaElement      },    
    { IDM_RADIOBUTTON,      &CLSID_HTMLInputElement         },
    { IDM_DROPDOWNBOX,      &CLSID_HTMLListElement          },        
    { IDM_MARQUEE,          &CLSID_HTMLMarqueeElement       },     
    { IDM_1D,               &CLSID_HTMLDivPosition          },        
    { IDM_HORIZONTALLINE,   &CLSID_HTMLHRElement            },          
    { IDM_IFRAME,           &CLSID_HTMLIFrame               },             
    { IDM_INSINPUTBUTTON,   &CLSID_HTMLInputElement         }, 
    { IDM_INSINPUTUPLOAD,   &CLSID_HTMLInputElement         },   
    { IDM_INSFIELDSET,      &CLSID_HTMLFieldSetElement      },    
    { IDM_PARAGRAPH,        &CLSID_HTMLParaElement          },        
    { IDM_INSINPUTIMAGE,    &CLSID_HTMLInputElement         }
//  BUGBUG: raminh
//          Per Rebecca and Sara this will be enabled soon,
//          must finalize naming convention and put it in Mshtmhstid.h
//  { IDM_HTMLAREA,         &CLSID_HTMLxxxxx                }

};


//+---------------------------------------------------------------------------
//
//  CInsertCommand Class
//
//----------------------------------------------------------------------------

class CInsertCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CInsertCommand))

    CInsertCommand(DWORD cmdId, ELEMENT_TAG_ID etagId, LPTSTR pstrAttribName, LPTSTR pstrAttribValue, CHTMLEditor * pEd);

    ~CInsertCommand();

    HRESULT ApplyCommandToSegment( IMarkupPointer   *pStart,
                                   IMarkupPointer   *pEnd,
                                   TCHAR            *pchVar,
                                   BOOL             fRemove = TRUE );

    void    SetAttributeValue(LPTSTR pstrAttribValue);

    BOOL IsValidOnControl();

protected:
    
    HRESULT PrivateExec( DWORD nCmdexecopt,
                  VARIANTARG * pvarargIn,
                  VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pCmd,
                         OLECMDTEXT * pcmdtext );

	ELEMENT_TAG_ID  _tagId;             // Tag to be used when applying the Command.
    BSTR            _bstrAttribName;    // Attribe name and value are used for creating
    BSTR            _bstrAttribValue;   // elements like <INPUT TYPE=RADIO>
};

//+---------------------------------------------------------------------------
//
//  CInsertParagraphCommand Class
//
//----------------------------------------------------------------------------

class CInsertParagraphCommand : public CInsertCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CInsertParagraphCommand));

    CInsertParagraphCommand(DWORD cmdId, CHTMLEditor * pEd)
        :  CInsertCommand(cmdId, TAGID_P, NULL, NULL, pEd)
    {
    }


    ~CInsertParagraphCommand()
    {
    }
    
protected:

    HRESULT PrivateExec( DWORD nCmdexecopt,
                         VARIANTARG * pvarargIn,
                         VARIANTARG * pvarargOut );
};

//+---------------------------------------------------------------------------
//
//  CInsertObjectCommand Class
//
//----------------------------------------------------------------------------

class CInsertObjectCommand : public CInsertCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CInsertObjectCommand));

    CInsertObjectCommand(DWORD cmdId, ELEMENT_TAG_ID etagId, LPTSTR pstrAttribName, LPTSTR pstrAttribValue, CHTMLEditor * pEd)
        :  CInsertCommand(cmdId, etagId, pstrAttribName, pstrAttribValue, pEd)
    {
    }


    ~CInsertObjectCommand()
    {
    }
    
protected:

    HRESULT PrivateExec( DWORD nCmdexecopt,
                         VARIANTARG * pvarargIn,
                         VARIANTARG * pvarargOut );
private:

    HRESULT     HandleInsertObjectDialog (HWND hwnd, DWORD * pdwResult, DWORD * pdwIntrinsicCmdId);
    HRESULT     SetAttributeFromClsid (CLSID * pClsid );
    void        MapObjectToIntrinsicControl (CLSID * pClsId, DWORD * pdwCmdId);
    void        OleUIMetafilePictIconFree( HGLOBAL hMetaPict );
};


#endif

