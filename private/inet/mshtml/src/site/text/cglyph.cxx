//+---------------------------------------------------------------------
//
//   File:      cglyph.cxx
//
//  Contents:   CGlyph Class which manages the information for glyphs and makes it available at rendering time.
//
//  Classes:    CGlyph
//              CGlyphTreeType
//
//------------------------------------------------------------------------
#include "headers.hxx"
#include "resource.h"
#include "cglyph.hxx"

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif


MtDefine(CGlyph,        CDoc,       "CGlyph")
MtDefine(CTreeObject,    CGlyph,     "CTreeObject")
MtDefine(CTreeList,      CTreeObject, "CTreeList")
MtDefine(GyphInfoType,  CTreeObject, "GyphInfoType")
    
    
//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::CGlyph()     
//
//  Synopsis:   Initializes the class' static data.  
//  
//----------------------------------------------------------------------------
CGlyph::CGlyph(CDoc * pDoc)
{
    _pDoc               = pDoc;
    s_levelSize [0]     = NUM_STATE_ELEMS; 
    s_levelSize [1]     = NUM_POS_ELEMS;
    s_levelSize [2]     = NUM_ALIGN_ELEMS;
    s_levelSize [3]     = NUM_ORIENT_ELEMS;
    _pchBeginDelimiter     = NULL;
    _pchEndDelimiter       = NULL;
    _pchEndLineDelimiter   = NULL;
    _pchDefaultImgURL      = NULL;
    _hEditResDLL           = NULL;
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::Init()     
//
//  Synopsis:   Makes the necessary memory allocations and initializations  
//  
//----------------------------------------------------------------------------
HRESULT
CGlyph::Init ()
{
    HRESULT     hr                  = S_OK;
    HINSTANCE   hResourceLibrary    = NULL;
    TCHAR       szBuffer [256];

    _XMLStack       = new CList;
    if (!_XMLStack)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    _gHashTable     = new CPtrBagCi <CGlyphTreeType *>;
    if (!_gHashTable)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = GetEditResourceLibrary (&hResourceLibrary);	
    if (hr || !hResourceLibrary)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    LoadString(hResourceLibrary, IDS_BEGIN_DELIMITER, szBuffer, ARRAY_SIZE(szBuffer));
    _pchBeginDelimiter = new TCHAR [_tcslen(szBuffer) + 1];
    if (!_pchBeginDelimiter)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    _tcscpy (_pchBeginDelimiter, szBuffer);

    LoadString(hResourceLibrary, IDS_END_DELIMITER, szBuffer, ARRAY_SIZE(szBuffer));
    _pchEndDelimiter = new TCHAR [_tcslen(szBuffer) + 1];
    if (!_pchEndDelimiter)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    _tcscpy (_pchEndDelimiter, szBuffer);

    LoadString(hResourceLibrary, IDS_END_LINE_DELIMITER, szBuffer, ARRAY_SIZE(szBuffer));
    _pchEndLineDelimiter = new TCHAR [_tcslen(szBuffer) + 1];
    if (!_pchEndLineDelimiter)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    _tcscpy (_pchEndLineDelimiter, szBuffer);

    
    hr = ConstructResourcePath (hResourceLibrary, szBuffer);
    if (hr)
        goto Cleanup;
    _tcscat (szBuffer, DEFAULT_IMG_NAME);
    _pchDefaultImgURL = new TCHAR [_tcslen(szBuffer)+1];
    if (!_pchDefaultImgURL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    _tcscpy (_pchDefaultImgURL, szBuffer);

Cleanup:
    FreeLibrary (_hEditResDLL);
    _hEditResDLL = NULL;
    RRETURN (hr);
}




HRESULT 
CGlyph::ConstructResourcePath (HINSTANCE hResourceLibrary, TCHAR szBuffer [])
{
    HRESULT     hr = S_OK;

    _tcscpy(szBuffer, _T("res://"));

    if (!GetModuleFileName(
            hResourceLibrary,
            szBuffer + _tcslen(szBuffer),
            pdlUrlLen - _tcslen(szBuffer) - 1))
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

#ifdef UNIX
    {
        TCHAR* p = _tcsrchr(szBuffer, _T('/'));
        if (p)
        {
            int iLen = _tcslen(++p);
            memmove(szBuffer + 6, p, sizeof(TCHAR) * iLen);
            szBuffer[6 + iLen] = _T('\0');
        }
    
    }
#endif

    _tcscat (szBuffer, _T("/"));

Cleanup:
    RRETURN (hr);
}


HRESULT 
CGlyph::GetEditResourceLibrary(
    HINSTANCE   *hResourceLibrary)
{
    
    if (!_hEditResDLL)
    {
        _hEditResDLL = MLLoadLibrary(_T("mshtmler.dll"), GetResourceHInst(), ML_CROSSCODEPAGE);
    }
    *hResourceLibrary = _hEditResDLL;

    if (!_hEditResDLL)
        return E_FAIL; // TODO: can we convert GetLastError() to an HRESULT?

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::~CGlyph()     
//
//  Synopsis:   First deletes the contents of the hash table, which is in 
//              charge of keeping track of glyphs for XML tags. Then, 
//              deletes the hash table, and finally iterates through the 
//              static array which keeps track of all the identified 
//              tags and deletes the contents of that as well.                 
//  
//              Note:   The sole putpose of _XMLStack is in fact to keep
//                      track of the information entered into the hash 
//                      table, because this implementation of a hash table
//                      does not have a handy way of iterating through it.
//
//----------------------------------------------------------------------------
CGlyph::~CGlyph()
{
    RemoveGlyphTableContents ();
    delete _pchBeginDelimiter;
    delete _pchEndDelimiter;
    delete _pchEndLineDelimiter;
    delete _pchDefaultImgURL;
    delete _gHashTable;
    delete _XMLStack;
    if (_hEditResDLL)
    {
        FreeLibrary (_hEditResDLL);
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::RemoveGlyphTableContents ()
//
//  Synopsis:   This method deletes all of the CGlyphTreeType structures
//              that we have. It begins by popping the entire stack 
//              of data that is contained in the hash table, and then
//              iterates through the static array which is reserved for
//              the known tags.
//
//----------------------------------------------------------------------------
HRESULT
CGlyph::RemoveGlyphTableContents ()
{
    CGlyphTreeType * XMLTree;
    int count;
 
    if (_XMLStack)
    {
        _XMLStack->Pop ((void**) &XMLTree);
        while (XMLTree)
        {
            delete XMLTree;
            _XMLStack->Pop ((void**) &XMLTree);
        }
    }
    for (count = 0; count <= ETAG_LAST; count++)
    {
        if ( _gIdentifiedTagArray [count] != NULL )
        {
            delete _gIdentifiedTagArray [count];
            _gIdentifiedTagArray [count] = NULL;
        }
    }
    return (S_OK);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::ReplaceGlyphTableContents
//
//  Synopsis:   This public method is called to purge the glyph information
//              and reconstruct the new data given a BSTR input.
//
//----------------------------------------------------------------------------
HRESULT 
CGlyph::ReplaceGlyphTableContents (BSTR inputStream)
{
    HRESULT hr;

    hr = THR( RemoveGlyphTableContents () );
    if (FAILED (hr))
        RRETURN (hr);
    hr = THR( ParseGlyphTable (inputStream, TRUE) );
    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::AddToGlyphTable
//
//  Synopsis:   This public method is used to insert new information
//              using a table formatted in a BSTR
//  
//----------------------------------------------------------------------------
HRESULT
CGlyph::AddToGlyphTable (BSTR inputStream)
{
    return ( ParseGlyphTable (inputStream, TRUE) );
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::RemoveFromGlyphTable 
//
//  Synopsis:   This public method is used to delete a rule from the table.
//              If this rule is not found ERROR_NOT_FOUND is returned.
//  
//----------------------------------------------------------------------------
HRESULT
CGlyph::RemoveFromGlyphTable (BSTR inputStream)
{
    return ( ParseGlyphTable (inputStream, FALSE) );
}



HRESULT 
CGlyph::AddSynthesizedRule (const TCHAR imgName [], ELEMENT_TAG eTag, GLYPH_STATE_TYPE eState, GLYPH_POSITION_TYPE ePos, 
                            GLYPH_ALIGNMENT_TYPE eAlign, GLYPH_ORIENTATION_TYPE eOrient, PTCHAR tagName )
{
    HRESULT                 hr                  = S_OK;
    CGlyphInfoType *        gInfo               = NULL;
    HINSTANCE               hResourceLibrary    = NULL;
    TCHAR                   pchImgURL [256];

    hr = GetEditResourceLibrary (&hResourceLibrary);
    if (hr || !hResourceLibrary)
        goto Cleanup;
    hr = ConstructResourcePath (hResourceLibrary, pchImgURL);
    if (hr)
        goto Cleanup;
    _tcscat (pchImgURL, imgName);

    gInfo = new CGlyphInfoType;
    if (gInfo == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    gInfo->pImageContext = NULL;
    gInfo->width = gInfo->height = gInfo->offsetX = gInfo->offsetY = DEFAULT_GLYPH_SIZE;
    gInfo->pchImgURL = new TCHAR [_tcslen(pchImgURL)+1];
    if (!gInfo->pchImgURL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    _tcscpy (gInfo->pchImgURL, pchImgURL);
    if (tagName == NULL)
    {
        hr = THR( InsertIntoTable (gInfo, eTag, eState, eAlign, ePos, eOrient, TRUE) );
    }
    else
    {
        hr = THR( InsertIntoTable (gInfo, tagName, eState, eAlign, ePos, eOrient, TRUE) );
    }
    if (FAILED (hr))
    {
        delete gInfo;
        goto Cleanup;
    }

Cleanup:
    FreeLibrary (_hEditResDLL);
    _hEditResDLL = NULL;
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::Exec   
//
//  Synopsis:   This method is used to implement various IDM command executions.
//              Nothing too complicated, except that it also takes charge of 
//              informing text whether it has shifted from a no-info state
//              to a state where CGlyph needs to be querried for glyph info.   
//  
//----------------------------------------------------------------------------
HRESULT
CGlyph::Exec(
        GUID * pguidCmdGroup,
        UINT idm,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    HRESULT                 hr          =   S_OK;

    switch (idm)
    {
    case IDM_SHOWALLTAGS:
        AddSynthesizedRule (_T("unknown.gif"), ETAG_NULL);
        hr = Exec (pguidCmdGroup, IDM_SHOWUNKNOWNTAGS, nCmdexecopt, pvarargIn, pvarargOut);
        if ( FAILED(hr) )
            break;
    case IDM_SHOWMISCTAGS:
        hr = Exec (pguidCmdGroup, IDM_SHOWALIGNEDSITETAGS, nCmdexecopt, pvarargIn, pvarargOut);
        if ( FAILED(hr) )
            break;
        hr = Exec (pguidCmdGroup, IDM_SHOWSCRIPTTAGS, nCmdexecopt, pvarargIn, pvarargOut);
        if ( FAILED(hr) )
            break;
        hr = Exec (pguidCmdGroup, IDM_SHOWSTYLETAGS, nCmdexecopt, pvarargIn, pvarargOut);
        if ( FAILED(hr) )
            break;
        hr = Exec (pguidCmdGroup, IDM_SHOWCOMMENTTAGS, nCmdexecopt, pvarargIn, pvarargOut);
        if ( FAILED(hr) )
            break;
        hr = Exec (pguidCmdGroup, IDM_SHOWAREATAGS, nCmdexecopt, pvarargIn, pvarargOut);
        if ( FAILED(hr) )
            break;
        hr = Exec (pguidCmdGroup, IDM_SHOWUNKNOWNTAGS, nCmdexecopt, pvarargIn, pvarargOut);
        if ( FAILED(hr) )
            break;
        hr = Exec (pguidCmdGroup, IDM_SHOWWBRTAGS, nCmdexecopt, pvarargIn, pvarargOut);
        if ( FAILED(hr) )
            break;
        AddSynthesizedRule (_T("abspos.gif"), ETAG_NULL, GST_DEFAULT, GPT_ABSOLUTE, GAT_DEFAULT, GOT_DEFAULT);
        break;
    case IDM_SHOWALIGNEDSITETAGS:
        AddSynthesizedRule (_T("leftalign.gif"), ETAG_NULL, GST_DEFAULT, GPT_DEFAULT, GAT_LEFT, GOT_DEFAULT);
        AddSynthesizedRule (_T("centeralign.gif"), ETAG_NULL, GST_DEFAULT, GPT_DEFAULT, GAT_CENTER, GOT_DEFAULT);
        AddSynthesizedRule (_T("rightalign.gif"), ETAG_NULL, GST_DEFAULT, GPT_DEFAULT, GAT_RIGHT, GOT_DEFAULT);
        break;
    case IDM_SHOWSCRIPTTAGS:
        AddSynthesizedRule (_T("script.gif"), ETAG_SCRIPT);
        break;
    case IDM_SHOWSTYLETAGS:
        AddSynthesizedRule (_T("style.gif"), ETAG_STYLE);
        break;
    case IDM_SHOWCOMMENTTAGS:
        AddSynthesizedRule (_T("comment.gif"), ETAG_COMMENT);
        AddSynthesizedRule (_T("comment.gif"), ETAG_RAW_COMMENT);
        break;
    case IDM_SHOWAREATAGS:
        AddSynthesizedRule (_T("area.gif"), ETAG_AREA);
        break;
    case IDM_SHOWUNKNOWNTAGS:
        AddSynthesizedRule (_T("unknown.gif"), ETAG_NULL, GST_DEFAULT, GPT_DEFAULT, GAT_DEFAULT, GOT_DEFAULT, DEFAULT_XML_TAG_NAME);
        break;
    case IDM_SHOWWBRTAGS:
        AddSynthesizedRule (_T("wordbreak.gif"), ETAG_WBR);
        AddSynthesizedRule (_T("br.gif"), ETAG_BR);
        break;
    case IDM_EMPTYGLYPHTABLE:
        {
            hr = THR(RemoveGlyphTableContents ());
            break;
        }
    case IDM_ADDTOGLYPHTABLE:
        if (!pvarargIn->bstrVal)
            break;
        hr = THR( AddToGlyphTable (pvarargIn->bstrVal) );
        break;
    case IDM_REMOVEFROMGLYPHTABLE:
        if (!pvarargIn->bstrVal)
            break;
        hr = THR( RemoveFromGlyphTable (pvarargIn->bstrVal) );
        break;
    case IDM_REPLACEGLYPHCONTENTS:
        if (!pvarargIn->bstrVal)
           break;
        hr = THR( ReplaceGlyphTableContents (pvarargIn->bstrVal) );
        break;
    default:
        hr = OLECMDERR_E_NOTSUPPORTED;
    }
    
    RRETURN (hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::ParseGlyphTable 
//
//  Synopsis:   This is the main method in the glyph table parsing from 
//              a BSTR. It begins by setting a PTCHAR that will walk 
//              down the BSTR as we are parsing it. Then, it enters a 
//              while loop until it runs into the end of the BSTR. 
//              Within the While loop, we first need to decide whether the
//              first entry is a tag ID or a tag name. If the first entry 
//              consists only of decimals, we conclude that is is a tag ID,
//              Then, we dispatch the parsing of the rest of the rule 
//              accordingly.
//
//----------------------------------------------------------------------------
HRESULT 
CGlyph::ParseGlyphTable (BSTR inputStream, BOOL addToTable)
{
    PTCHAR      pchInStream = inputStream;
    PTCHAR      pchThisSection;
    HRESULT     hr = S_OK;

    if (_tcslen (pchInStream) < _tcslen (_pchBeginDelimiter))
        goto Cleanup;
    pchInStream = _tcsstr (pchInStream, _pchBeginDelimiter);
    while (pchInStream)
    {
        GetThisSection (pchInStream, pchThisSection);
        if (pchThisSection == NULL)    //If the tag ID/name field is empty, we make it default
        {
            pchThisSection = new TCHAR [5];
            if (!pchThisSection)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            _itot (ETAG_UNKNOWN, pchThisSection, 10);
        }
        if (_tcsspn (pchThisSection, DECIMALS) == _tcsclen(pchThisSection))   //If this rule specifies a tag ID
        {
            IDParse (pchInStream, pchThisSection, addToTable);    
        }
        else    //If this rule specifies a tag name
        {
            XMLParse (pchInStream, pchThisSection, addToTable);
        }
        delete pchThisSection;
        // Goto next rule
        if (pchInStream)           
            pchInStream = _tcsstr (pchInStream, _pchEndLineDelimiter);
        if (pchInStream)
            pchInStream = pchInStream + _tcsclen (_pchEndLineDelimiter);
        if (pchInStream)
            pchInStream = _tcsstr (pchInStream, _pchBeginDelimiter);
    }

Cleanup:
    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::ParseBasicInfo 
//
//  Synopsis:   This method parses the basic Glyph info which is common
//              to both tags with identified ID's and others (i.e. XML's).
//
//----------------------------------------------------------------------------
HRESULT
CGlyph::ParseBasicInfo (PTCHAR & pchInStream, BasicGlyphInfoType & newRule)
{
    HRESULT             hr = S_OK;
    LONG                temp;

    //Initialize the Fields
    newRule.eState = GST_DEFAULT;
    newRule.eAlign = GAT_DEFAULT;
    newRule.ePos = GPT_DEFAULT;
    newRule.eOrient = GOT_DEFAULT;
    newRule.width = newRule.height = newRule.offsetX = newRule.offsetY = DEFAULT_GLYPH_SIZE;

    GetThisSection (pchInStream, newRule.pchImgURL);
    if (newRule.pchImgURL == NULL)
    {
        newRule.pchImgURL = new TCHAR [_tcsclen(_pchDefaultImgURL)+1];
        if (!newRule.pchImgURL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _tcscpy(newRule.pchImgURL, _pchDefaultImgURL);
        goto Cleanup;
    }


    if (!NextIntSection (pchInStream, temp))
        goto Cleanup;
    newRule.eState  =   (GLYPH_STATE_TYPE)          temp;

    if (!NextIntSection (pchInStream, temp))
        goto Cleanup;
    newRule.eAlign  =   (GLYPH_ALIGNMENT_TYPE)      temp;

    if (!NextIntSection (pchInStream, temp))
        goto Cleanup;
    newRule.ePos    =   (GLYPH_POSITION_TYPE)       temp;

    if (!NextIntSection (pchInStream, temp))
        goto Cleanup;
    newRule.eOrient =   (GLYPH_ORIENTATION_TYPE)    temp;

    if (!NextIntSection (pchInStream, newRule.width))
        goto Cleanup;
    if (!NextIntSection (pchInStream, newRule.height))
        goto Cleanup;
    if (!NextIntSection (pchInStream, newRule.offsetX))
        goto Cleanup;
    if (!NextIntSection (pchInStream, newRule.offsetY))
        goto Cleanup;

Cleanup:
    return (S_OK);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::IDParse 
//
//  Synopsis:   This nethod takes care of the parsing of rules where the
//              tag is identified by its ID. It begins by setting the 
//              correct tag ID, and then parsing the rest of the info
//              which is common to rules with ID's and names, and finally
//              enters the information.
//  
//----------------------------------------------------------------------------
HRESULT
CGlyph::IDParse (PTCHAR & pchInStream, PTCHAR & pchThisSection, BOOL addToTable)
{
    HRESULT             hr;
    PTCHAR              pchTagName;
    ELEMENT_TAG_ID      tagID = (ELEMENT_TAG_ID)  _ttol(pchThisSection);
    extern ELEMENT_TAG  ETagFromTagId ( ELEMENT_TAG_ID tagID );

    if (   tagID <= TAGID_NULL
        || tagID == TAGID_UNKNOWN
        || tagID >= TAGID_COUNT
       )
    {
        hr = THR( _pDoc->GetNameForTagID (tagID, &pchTagName));
        if (hr || !pchTagName)
            goto Cleanup;
        hr = THR( XMLParse (pchInStream, pchTagName, addToTable) );
    }
    else
    {
        IDGlyphTableType    newRule;
        hr = THR( ParseBasicInfo (pchInStream, newRule.basicInfo) ); 
        if (hr)
            goto Cleanup;
        newRule.eTag = ETagFromTagId(tagID);
        hr = THR( NewEntry (newRule, addToTable) );
    }

Cleanup:
    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::XMLParse 
//
//  Synopsis:   This nethod takes care of the parsing of rules where the
//              tag is identified by its name. It begins by setting the 
//              correct tag name, and then parsing the rest of the info
//              which is common to rules with ID's and names, and finally
//              enters the information.
//
//----------------------------------------------------------------------------
HRESULT
CGlyph::XMLParse (PTCHAR & pchInStream, PTCHAR & pchThisSection, BOOL addToTable)
{
    XMLGlyphTableType   newRule;
    HRESULT             hr;

    newRule.pchTagName = new TCHAR [_tcsclen(pchThisSection)+1];
    if (!newRule.pchTagName)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    _tcscpy(newRule.pchTagName, pchThisSection);
    hr = THR( ParseBasicInfo (pchInStream, newRule.basicInfo) ); 
    if (hr)
        goto Cleanup;
    hr = THR( NewEntry (newRule, addToTable) );

Cleanup:
    delete newRule.pchTagName;
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::NextIntSection 
//
//  Synopsis:   This method parses between a _pchBeginDelimiter and _pchEndDelimiter,
//              expecting to find an integer there. If this section is not 
//              empty, the 'result' parameter is set to the integer 
//              translation of the string, and TRUE is returned. If not,
//              'result' is left unchanges, and FALSE is returned.
//  
//              Note:   Both the return value and the setting of
//                      'result' are used by 'ParseBasicInfo,' depending
//                      on whether we need to typecase a dummy variable.
//
//----------------------------------------------------------------------------
BOOL
CGlyph::NextIntSection (PTCHAR & pchInStream, LONG & result)
{
    PTCHAR  pchThisSection  = NULL;
    BOOL    f_rVal          = FALSE;

    if (pchInStream == NULL) // We leave and the default value remains
    {
        goto Cleanup;
    }
    GetThisSection (pchInStream, pchThisSection);
    if (pchThisSection == NULL)
    {
        goto Cleanup;
    }
    result = _ttol(pchThisSection);
    f_rVal = TRUE;

Cleanup:
    delete pchThisSection;
    return (f_rVal);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::GetThisSection 
//
//  Synopsis:   This method is used to retrieve a section in the input 
//              stream that we are parsing. What is does is localize the
//              the next input field, and then constructs a TCHAR string
//              out of that.
//  
//              Note:   In case the caller does not need to preserve 
//                      'newString', the latter should be deleted 
//                      when no longer need by the caller.
//
//----------------------------------------------------------------------------
HRESULT
CGlyph::GetThisSection (PTCHAR & pchSectionBegin, PTCHAR & pchNewString)
{
    PTCHAR      pchSectionEnd;
    PTCHAR      pchRuleEnd;
    int         beginLen;
    int         endLen;
    HRESULT     hr = S_OK;

    AssertSz(pchSectionBegin, "Parser Should Have Popped Out Before Passing In a Null"); 
    pchNewString = NULL;
    pchRuleEnd      = _tcsstr (pchSectionBegin, _pchEndLineDelimiter);
    if (!pchRuleEnd)    //  Messed up syntax - Try to recover cleanly
    {
        goto Cleanup;
    }
    pchSectionEnd = _tcsstr (pchSectionBegin, _pchBeginDelimiter);
    if (!pchSectionEnd || pchRuleEnd < pchSectionEnd)    // The next non-empty field is for another rule 
    {
        goto Cleanup;
    }

    // Now, we can begin
    pchSectionBegin = pchSectionEnd;
    beginLen = _tcsclen(pchSectionBegin);
    AssertSz (pchSectionBegin && CompareUpTo (pchSectionBegin, _pchBeginDelimiter, _tcsclen(_pchBeginDelimiter)) == 0, "Illegal Input Stream");
    pchSectionBegin = pchSectionBegin + _tcsclen (_pchBeginDelimiter);
    pchSectionEnd = _tcsstr (pchSectionBegin, _pchEndDelimiter);
    beginLen = _tcsclen(pchSectionBegin);
    endLen = _tcsclen(pchSectionEnd);
    if (beginLen == endLen) //Check if this field is empty
    {
        goto Cleanup;
    }
    pchNewString = new TCHAR [beginLen-endLen+1];
    if (!pchNewString)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    _tcsncpy (pchNewString, pchSectionBegin, beginLen-endLen);
    pchNewString[beginLen-endLen] = '\0';
    pchSectionBegin = pchSectionEnd + _tcsclen (_pchEndDelimiter);
    
Cleanup:
    return (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::CompareUpTo 
//
//  Synopsis:   This method return TRUE or FALSE depending on whether 
//              two strings are identical up to a certain number of 
//              characters.
//
//----------------------------------------------------------------------------
int
CGlyph::CompareUpTo (PTCHAR first, PTCHAR second, int numToComp)
{
    int count;
    int firstLen = _tcslen (first);
    int secondLen = _tcslen (second);

    for (count = 0; count < numToComp; count++)
        if (((count >= firstLen) || (count >= secondLen)) || (first[count] != second[count]))
            break;
    if (count == numToComp)
        return 0;
    else return 1;
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::InitGInfo 
//
//  Synopsis:   Creates a CGlyphInfoType and sets it to its default values,
//              specified in basicInfo.
//  
//----------------------------------------------------------------------------
HRESULT 
CGlyph::InitGInfo (pCGlyphInfoType & gInfo, BasicGlyphInfoType & basicInfo)
{
    HRESULT hr = NOERROR;

    gInfo = new CGlyphInfoType;
    if (gInfo == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    gInfo->pImageContext    = NULL;
    gInfo->pchImgURL           = basicInfo.pchImgURL;
    gInfo->width            = basicInfo.width;
    gInfo->height           = basicInfo.height;
    gInfo->offsetX          = basicInfo.offsetX;
    gInfo->offsetY          = basicInfo.offsetY;

Cleanup:
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::NewEntry
//
//  Synopsis:   This version of NewEntry is used to enter a new struct into
//              the database of Glyph info, where the name of a tag has been
//              specified.
//
//----------------------------------------------------------------------------
HRESULT
CGlyph::NewEntry (XMLGlyphTableType & gTableElem, BOOL addToTable )
{
    HRESULT         hr = S_OK;
    CGlyphInfoType * gInfo;

    hr = THR( InitGInfo (gInfo, gTableElem.basicInfo) );
    if ( FAILED (hr) )
        goto Cleanup;
    hr = THR( InsertIntoTable (gInfo, gTableElem.pchTagName, gTableElem.basicInfo.eState, gTableElem.basicInfo.eAlign, 
        gTableElem.basicInfo.ePos, gTableElem.basicInfo.eOrient, addToTable) );
    if (!addToTable)
        delete gInfo;

Cleanup:
    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     
//
//  Synopsis:   This version of NewEntry is used to enter a new struct into
//              the database of Glyph info, where the tag is identified
//              by its tag ID.
//
//----------------------------------------------------------------------------
HRESULT
CGlyph::NewEntry(IDGlyphTableType & gTableElem, BOOL addToTable)
{
    HRESULT hr = NOERROR;

    CGlyphInfoType * gInfo;
    hr = THR( InitGInfo (gInfo, gTableElem.basicInfo) );
    if ( FAILED (hr) )
        goto Cleanup;
    hr = THR( InsertIntoTable (gInfo, gTableElem.eTag, gTableElem.basicInfo.eState, gTableElem.basicInfo.eAlign, 
        gTableElem.basicInfo.ePos, gTableElem.basicInfo.eOrient, addToTable) );

Cleanup:
    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::InsertIntoTable 
//
//  Synopsis:   This method adds a new entry into the database of 
//              CGlyphInfoTypes when the tag has been identified by its name.
//              First, if the tag name is empty, or null it is set to the 
//              default value of ETAG_NULL. Next, if we attempt to resolve 
//              this tag name with a tag ID. If we have been able to attach
//              the tag name with an ID, we enter the new info into the
//              static array that contains info for tags with identified
//              ID's. If that is not the case, we enter the info into
//              the hash table which is reserved for tags that we can 
//              not resolve into an ID.
//
//----------------------------------------------------------------------------
HRESULT 
CGlyph::InsertIntoTable (CGlyphInfoType * gInfo, PTCHAR pchTagName, GLYPH_STATE_TYPE eState, GLYPH_ALIGNMENT_TYPE eAlign, 
                         GLYPH_POSITION_TYPE ePos, GLYPH_ORIENTATION_TYPE eOrient,
                         BOOL addToTable)
{
    CGlyphTreeType *    infoTable;
    HRESULT             hr;

    ELEMENT_TAG         eTag;
    hr = THR( AttemptToResolveTagName (pchTagName, eTag) );
    if (FAILED (hr))
    {
        delete gInfo;
        goto Cleanup;
    }
    
    //
    // BUGBUG - is compare with DEFAULT_XML_TAG_NAME still required ?
    //
    if ( !IsGenericTag (eTag) && _tcscmp (pchTagName, DEFAULT_XML_TAG_NAME) != 0)
    {
        hr = THR( InsertIntoTable (gInfo, eTag, eState, eAlign, ePos, eOrient, addToTable) );
        goto Cleanup;
    }

    infoTable = _gHashTable->GetCi(pchTagName);

    if (infoTable == NULL)
    {
        infoTable = new CGlyphTreeType();
        if (infoTable == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        
        hr = THR( _gHashTable->SetCi (pchTagName, infoTable) );
        if (FAILED (hr) )
        {
            delete infoTable;
            goto Cleanup;
        }
        _XMLStack->Push ((void**)infoTable);
    }
    if (infoTable != NULL)
    {
        hr = THR( infoTable->AddRule (gInfo, eState, eAlign, ePos, eOrient, addToTable, this) );
    }

Cleanup:
    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::InsertIntoTable 
//
//  Synopsis:   This method enters a CGlyphInfoType struct into the static
//              array. If ever an unidentified tag ID is passed in, 
//              this entry is ignored.
//  
//----------------------------------------------------------------------------
HRESULT 
CGlyph::InsertIntoTable (CGlyphInfoType * gInfo, ELEMENT_TAG eTag, GLYPH_STATE_TYPE eState, GLYPH_ALIGNMENT_TYPE eAlign, 
                         GLYPH_POSITION_TYPE ePos, GLYPH_ORIENTATION_TYPE eOrient,
                         BOOL addToTable)
{
    CGlyphTreeType *    infoTable;
    HRESULT             hr = S_OK;

    if (eTag >= ETAG_LAST)
    {
        delete gInfo;
        goto Cleanup;
    }
    if (_gIdentifiedTagArray[eTag] == NULL)
    {
        infoTable = new CGlyphTreeType();
        if (infoTable == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _gIdentifiedTagArray[eTag] = infoTable;
    }
    else 
    {
        infoTable = _gIdentifiedTagArray[eTag];
    }
    if (infoTable != NULL)
    {
        hr = THR( infoTable->AddRule (gInfo, eState, eAlign, ePos, eOrient, addToTable, this) );
    }

Cleanup:
    RRETURN (hr);

}







//+---------------------------------------------------------------------------
//
//  Member:     CALLBACK OnImgCtxChange
//
//  Synopsis:   This is the callback function that is used to invalidate 
//              the page when a new image has finished downloading for the 
//              first time.
//
//----------------------------------------------------------------------------
void CALLBACK 
CGlyph::OnImgCtxChange( VOID * pvImgCtx, VOID * pv )
{
    ((CDoc *)pv)->GetView()->Invalidate();
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::InitRenderTagInfo 
//
//  Synopsis:   None
//
//----------------------------------------------------------------------------
HRESULT 
CGlyph::InitRenderTagInfo (CGlyphRenderInfoType * renderTagInfo)
{
    HRESULT hr = S_OK;

    if (renderTagInfo == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    renderTagInfo->pImageContext = NULL;
    renderTagInfo->width = 0;
    renderTagInfo->height = 0;
    renderTagInfo->offsetX = 0;
    renderTagInfo->offsetY = 0;

Cleanup:
    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::CompleteInfoProcessing 
//
//  Synopsis:   Once the CGlyphInfoType structure has been found, this method
//              is used to set up the information to send it back to 
//              rendering.
//  
//              Note:   This method unfifes the code path for XML and normal
//                      tags.
//
//----------------------------------------------------------------------------
HRESULT
CGlyph::CompleteInfoProcessing (CGlyphInfoType * localInfo, CGlyphRenderInfoType * pTagInfo, void * invalidateInfo)
{
    HRESULT hr = S_OK;

    AssertSz (localInfo, "This point should only be reached if a valid query was made");
        
    // Bitmap handle has not been loaded yet
    // i.e. this is the first time this image is requested
    if (localInfo->pImageContext == NULL) 
    {
         hr = THR( GetImageContext (localInfo, invalidateInfo, localInfo->pImageContext) );
         if (FAILED (hr) )
             goto Cleanup;
    }
    pTagInfo->pImageContext = localInfo->pImageContext;
    pTagInfo->width = localInfo->width;
    pTagInfo->height = localInfo->height;
    pTagInfo->offsetX = localInfo->offsetX;
    pTagInfo->offsetY = localInfo->offsetY;

Cleanup:
    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::AttemptToResolveTagName 
//
//  Synopsis:   Retrieves that tag info given a tag name. If the tag name
//              is resolvable into an ID. the the codepath goes to the ID-
//              identified glyphs. If not, we go on to looking in the hash
//              table.
//  
//              Note:   The attempt to resolve tag names into ID's is the 
//                      same as the point where new info is entered.
//
//----------------------------------------------------------------------------
    

HRESULT
CGlyph::AttemptToResolveTagName (PTCHAR pchTagName, ELEMENT_TAG & eTag)
{
    HRESULT hr  = S_OK;

    AssertSz (pchTagName, "Tag name cannot be NULL");
    eTag = ETAG_NULL;
    if (pchTagName == NULL)
    {
        eTag = ETAG_UNKNOWN;
    }
    else
    {
        //
        // If the name has a colon in it - assume it's an XML namespace tag.
        //
        if ( _tcsstr( pchTagName, _T(":") ) > 0 )
        {
            eTag = ETAG_GENERIC;
        }
        else
            eTag = EtagFromName (pchTagName, _tcslen(pchTagName));
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::GetTagInfo 
//
//  Synopsis:   Retrieves that tag info given a ptp. It first searches for
//              a location in the data where the the ID's resolve (an entry
//              for the specific tag or an entry for the dafaults). Then,
//              if we can find the info, the return struct is set.
//  
//              Note:   The attempt to resolve tag names into ID's is the 
//                      same as the point where new info is entered.
//
//----------------------------------------------------------------------------



HRESULT
CGlyph::GetTagInfo (CTreePos * ptp, GLYPH_ALIGNMENT_TYPE eAlign, 
                    GLYPH_POSITION_TYPE ePos, GLYPH_ORIENTATION_TYPE eOrient, 
                    void * invalidateInfo, CGlyphRenderInfoType * pTagInfo)
{  
    CGlyphTreeType *            infoTree        = NULL;
    CGlyphInfoType *            localInfo       = NULL;
    HRESULT                     hr              = S_OK;
    ELEMENT_TAG                 eTag;
    GLYPH_STATE_TYPE            eState;
    CTreeNode *                 pNode;

    pTagInfo->pImageContext = NULL;

    if ( !ptp || !ptp->IsNode() )
        goto Cleanup;
    
    pNode = ptp->Branch ();
    eTag = pNode->Tag();

    if (  (eTag == ETAG_NULL)
        || !ptp->IsEdgeScope()
        || (   ptp->IsEndNode()
            && pNode->Element()->IsNoScope()
           )
       )
        goto Cleanup;

    eState = (GLYPH_STATE_TYPE)!(ptp->IsBeginElementScope());
    if (eTag == ETAG_UNKNOWN || IsGenericTag (eTag))        //. If it is a generic (i.e. known XML tag), we go off to the XML tag search path
    {
        hr = THR( GetXMLTagInfo (ptp, eState, eAlign, ePos, eOrient, invalidateInfo, pTagInfo) );
        goto Cleanup;
    }

    infoTree = _gIdentifiedTagArray[eTag];  // First, we try to see if we have info for this specific tag
    if (infoTree)
    {
        hr = THR( infoTree->GetGlyphInfo (ptp, localInfo, eState, eAlign, ePos, eOrient) );
        if (FAILED (hr))
            goto Cleanup;
    }
    
    if (!localInfo) // If we don't have info for the specific tag
    {
        infoTree = _gIdentifiedTagArray[ETAG_UNKNOWN];  // We first try the ETAG default
        if (infoTree)
        {
            hr = THR( infoTree->GetGlyphInfo (ptp, localInfo, eState, eAlign, ePos, eOrient) );
            if (FAILED (hr))
                goto Cleanup;
        }

        if (!localInfo) // If we don't unfiy with an ETAG default, our final try is for the general default
        {
            hr = THR( AttemptFinalDefault (ptp, localInfo, eState, eAlign, ePos, eOrient) );
            if (FAILED (hr))
                goto Cleanup;
        }
    }

    if (!localInfo)
        goto Cleanup;

    hr = THR( CompleteInfoProcessing (localInfo, pTagInfo, invalidateInfo) );

Cleanup:
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::GetTagInfo 
//
//  Synopsis:   Retrieves that tag info given an ptp, and knowing that it 
//              point to a generic tag.
//  
//----------------------------------------------------------------------------
HRESULT
CGlyph::GetXMLTagInfo (CTreePos * ptp, GLYPH_STATE_TYPE eState, GLYPH_ALIGNMENT_TYPE eAlign, 
                    GLYPH_POSITION_TYPE ePos, GLYPH_ORIENTATION_TYPE eOrient, 
                    void * invalidateInfo, CGlyphRenderInfoType * pTagInfo)
{  
    CGlyphTreeType *            infoTree        = NULL;
    CGlyphInfoType *            localInfo       = NULL;
    CElement*                   pElement;
    HRESULT                     hr              = S_OK;
    TCHAR strLookupName[256];
    

    if (!ptp->Branch())
        goto Cleanup;
    pElement = ptp->Branch()->Element();

    //
    // Construct a string that is NameSpace:TagName
    //
    strLookupName[0] = 0;
    if (pElement->Namespace())
    {
        _tcscat(strLookupName , pElement->Namespace() );
        _tcscat(strLookupName, _T(":") );
    }
    _tcscat(strLookupName, pElement->TagName() );

    infoTree = _gHashTable->GetCi( strLookupName ); // Attempt to find entry for specific XML tag
    if (infoTree)
    {
        hr = THR( infoTree->GetGlyphInfo (ptp, localInfo, eState, eAlign, ePos, eOrient) );
        if (FAILED (hr))
            goto Cleanup;
    }


    if (!localInfo) // Attempt to find default entry for all XML's
    {
        infoTree = _gHashTable->GetCi(DEFAULT_XML_TAG_NAME);
        if (infoTree)
        {
            hr = THR( infoTree->GetGlyphInfo (ptp, localInfo, eState, eAlign, ePos, eOrient) );
            if (FAILED (hr))
                goto Cleanup;
        }

        if (!localInfo) // Attempt final default for all tags (ETAGS and XMLs)
        {
            hr = THR( AttemptFinalDefault (ptp, localInfo, eState, eAlign, ePos, eOrient) );
            if (FAILED (hr))
                goto Cleanup;
        }
    }

    if (!localInfo)
        goto Cleanup;

    hr = THR( CompleteInfoProcessing (localInfo, pTagInfo, invalidateInfo) );

Cleanup:
    RRETURN (hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::AttemptFinalDefault 
//
//  Synopsis:   This is the final default condition, where we tet if the table
//              contains an entry for ETAG_NULL, which is the final default
//              that everything falls through to.
//  
//----------------------------------------------------------------------------
HRESULT
CGlyph::AttemptFinalDefault (CTreePos * ptp, pCGlyphInfoType & localInfo, GLYPH_STATE_TYPE eState, GLYPH_ALIGNMENT_TYPE eAlign,
                             GLYPH_POSITION_TYPE ePos, GLYPH_ORIENTATION_TYPE eOrient)
{
    CGlyphTreeType *    infoTable   = NULL;
    HRESULT             hr          = S_OK;

    infoTable = _gIdentifiedTagArray[ETAG_NULL];
    if (infoTable)
        hr = THR( infoTable->GetGlyphInfo (ptp, localInfo, eState, eAlign, ePos, eOrient) );

    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::GetImageContext 
//
//  Synopsis:   Creates the image context, and sets the callback function
//              after having created the invalidation stack and mande the 
//              first entry to it.   
//  
//              Note:   We pass in a pointer to the gInfo into the callback.
//                      This is an elegant way of keeping track of what needs
//                      to be invalidated when an image is done loading,
//                      because whenever we want to display an image and it
//                      is not loaded, we know the CGlyphInfoType, so we 
//                      can add the invalidation info in the stack.
//
//----------------------------------------------------------------------------
HRESULT
CGlyph::GetImageContext (CGlyphInfoType * gInfo, void * pvCallback, pIImgCtx & newImageContext)
{
    HRESULT     hr      = S_OK;

    newImageContext = NULL;
    hr = THR( CoCreateInstance(CLSID_IImgCtx, NULL, CLSCTX_INPROC_SERVER,
                          IID_IImgCtx, (LPVOID*)&newImageContext) );
    if (SUCCEEDED(hr))
    {
        hr = THR( newImageContext->Load(gInfo->pchImgURL, IMGCHG_COMPLETE) );
        if ( SUCCEEDED( hr ))
        {
            hr = THR( newImageContext->SetCallback( OnImgCtxChange, _pDoc ) );
        }
        if ( SUCCEEDED( hr ))
        {
            hr = THR( newImageContext->SelectChanges( IMGCHG_COMPLETE, 0, TRUE) );
        }
        if ( FAILED( hr ))
        {
            newImageContext->Release();
            return hr;
        }
    }		

    RRETURN (hr); 
}




//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::CGlyphTreeType::CGlyphTreeType 
//
//  Synopsis:   Creates the frst level of the tree and initializes it.
//  
//----------------------------------------------------------------------------
CGlyph::CGlyphTreeType::CGlyphTreeType ()
{
    _infoTree = new CTreeList (s_levelSize [0]);
    if (_infoTree == NULL)
    {
        goto Cleanup;
    }

Cleanup:
    return;
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::CGlyphTreeType::~CGlyphTreeType 
//
//----------------------------------------------------------------------------
CGlyph::CGlyphTreeType::~CGlyphTreeType ()
{
    delete _infoTree;
}






//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::CGlyphTreeType::TransformInfoToArray
//
//  Synopsis:   Converts glyph access information into indexes along the
//              the various levels of the tree.
//  
//              Note:   This is one of the only two methods that needs to be changed to
//                      modify the searching priority.
//
//----------------------------------------------------------------------------
HRESULT
CGlyph::CGlyphTreeType::TransformInfoToArray (GLYPH_STATE_TYPE eState, GLYPH_ALIGNMENT_TYPE eAlign, GLYPH_POSITION_TYPE ePos, 
                                     GLYPH_ORIENTATION_TYPE eOrient, int indexArray [])
{
    indexArray [TREEDEPTH_STATE]            = eState;
    indexArray [TREEDEPTH_POSITIONINING]    = ePos;
    indexArray [TREEDEPTH_ALIGNMENT]        = eAlign;
    indexArray [TREEDEPTH_ORIENTATION]      = eOrient;
    return (S_OK);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::CGlyphTreeType::ComputeLevelIndex
//
//  Synopsis:   Takes a ptpt and retrieves the specific details that are 
//              are needed to iterate through the next level in the tree.
//              This allows us to querry for specific info from the 
//              CTreePos only as needed.
//
//----------------------------------------------------------------------------
HRESULT
CGlyph::CGlyphTreeType::ComputeLevelIndex (CTreePos * ptp, int index [], int levelCount)
{
    HRESULT             hr          = S_OK;
    CTreeNode *         pNode       = ptp->Branch ();
    htmlControlAlign    alignment;
    CFlowLayout *       pLayout;
    LCID                curKbd;

    AssertSz ((levelCount >= 0) && (levelCount < NUM_INFO_LEVELS), "levelCount needs to be within bounds");
    AssertSz (index [levelCount] == COMPUTE, "index [levelCount] == COMPUTE is why we called this in the first place");

    switch (levelCount)
    {
    case TREEDEPTH_STATE : 
        if ( ptp->IsEndElementScope () )
            index [levelCount] = GST_CLOSE;
        else
            index [levelCount] = GST_OPEN;
        break;
    case TREEDEPTH_POSITIONINING : 
        if ( pNode->IsPositionStatic () )
            index [levelCount] = GPT_STATIC;
        else if ( pNode->IsAbsolute () )
            index [levelCount] = GPT_ABSOLUTE;
        else if ( pNode->IsRelative () )
            index [levelCount] = GPT_RELATIVE;
        else 
            index [levelCount] = GPT_DEFAULT;
        break;
    case TREEDEPTH_ALIGNMENT:
        alignment   = pNode->GetSiteAlign ();

        if ( alignment == htmlBlockAlignLeft )
            index [levelCount] = GAT_LEFT;
        else if ( alignment == htmlBlockAlignCenter )
            index [levelCount] = GAT_CENTER;
        else if ( alignment == htmlBlockAlignRight )
            index [levelCount] = GAT_RIGHT;
        else 
            index [levelCount] = GAT_DEFAULT;
        break;
    case TREEDEPTH_ORIENTATION:
        pLayout     = pNode->GetFlowLayout();
        if (!pLayout)
        {
            index [levelCount] = GOT_DEFAULT;
            break;
        }
        curKbd      = LOWORD(GetKeyboardLayout(0));

        if ( pLayout->GetVertical() )   // Vertical text ==> UP-DOWN or DOWN-UP
        {
            if(IsRtlLCID(curKbd))       // DOWN-UP
                index [levelCount] = GOT_BOTTOM_TO_TOP;
            else 
                index [levelCount] = GOT_TOP_TO_BOTTOM;
        }
        else                            // Horizontal text ==> LEFT-RIGHT or RIGHT-LEFT
        {
            if(IsRtlLCID(curKbd))       // RIGHT-LEFT
                index [levelCount] = GOT_RIGHT_TO_LEFT;
            else 
                index [levelCount] = GOT_LEFT_TO_RIGHT;
        }
        break;
    default:
        AssertSz (0, "We should always be at a level that we know about and can resolve");
    }
    
    return (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::CGlyphTreeType::AddRule
//              CGlyph::CGlyphTreeType::InsertIntoTree
//
//  Synopsis:   This method adds a new rule (CGlyphInfoType struct) into
//              a tree. It searches for the location, creating levels 
//              if necessary, and inserting the new info.
//  
//              Note:   Rules that map on to the same location are resolved
//                      by deleting the old rule and replacing it by the new
//                      one. This allows for a handy way of overwriting
//                      a rule.
//
//----------------------------------------------------------------------------
HRESULT 
CGlyph::CGlyphTreeType::AddRule (CGlyphInfoType * gInfo, GLYPH_STATE_TYPE eState, GLYPH_ALIGNMENT_TYPE eAlign,
                  GLYPH_POSITION_TYPE ePos, GLYPH_ORIENTATION_TYPE eOrient, BOOL addToTable, CGlyph * glyphTable)
{
    int indexArray [NUM_INFO_LEVELS];
    HRESULT     hr = S_OK;

    TransformInfoToArray (eState, eAlign, ePos, eOrient, indexArray);
    hr = THR( InsertIntoTree (gInfo, indexArray, addToTable, glyphTable) );
    if (hr == S_OK)
        glyphTable->_pDoc->ForceRelayout();
    RRETURN (hr);
}


HRESULT
CGlyph::CGlyphTreeType::InsertIntoTree (CGlyphInfoType * gInfo, int index [], BOOL addToTable, CGlyph * glyphTable)
{
    int         levelCount;
    CTreeList *     thisLevel = _infoTree;
    CTreeList *     newLevel;
    HRESULT     hr = S_OK;
    
    for (levelCount = 0; levelCount < NUM_INFO_LEVELS-1; levelCount ++)
    {
        if ((*thisLevel)[index[levelCount]] == NULL)
        {
            if (!addToTable)
            {
                goto Cleanup;
            }
            newLevel = new CTreeList(s_levelSize[levelCount+1]);
            if (newLevel == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            (*thisLevel)[index[levelCount]] = newLevel;
        }
        else 
        {
            newLevel = DYNCAST(CTreeList, (*thisLevel)[index[levelCount]]);    
        }
        thisLevel = newLevel;
    }
    // Rule collisions are handled by preserving the last one
    if ((*thisLevel)[index[levelCount]] != NULL)
    {
        delete (*thisLevel)[index[levelCount]]; 
        (*thisLevel)[index[levelCount]] = NULL;
    }
    else if (!addToTable)       //Tried to delete non-existing rule
    {
        goto Cleanup;
    }
    if (addToTable)
    {
        (*thisLevel)[index[levelCount]] = gInfo;
    }

Cleanup:
    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CGlyph::CGlyphTreeType::GetGlyphInfo
//              CGlyph::CGlyphTreeType::GetFromTree
//
//  Synopsis:   Retrieves the pointer to a CGlyphInfoType struct from a tree.
//              The search performed is depth-first. We begin by looking 
//              for the exact rule specified, and as we climb back up
//              We try each default.
//
//----------------------------------------------------------------------------
HRESULT 
CGlyph::CGlyphTreeType::GetGlyphInfo (CTreePos * ptp, pCGlyphInfoType & gInfo, GLYPH_STATE_TYPE eState, GLYPH_ALIGNMENT_TYPE eAlign,
                             GLYPH_POSITION_TYPE ePos, GLYPH_ORIENTATION_TYPE eOrient)
{
    int indexArray [NUM_INFO_LEVELS];
    HRESULT     hr = S_OK;

    TransformInfoToArray (eState, eAlign,  ePos, eOrient, indexArray);
    hr = THR( GetFromTree (ptp, gInfo, indexArray) );
    RRETURN (hr);
}


HRESULT
CGlyph::CGlyphTreeType::GetFromTree (CTreePos * ptp, pCGlyphInfoType & gInfo, int index [])
{
    int         levelCount = 0;
    CTreeObject *  thisLevel = DYNCAST(CTreeList, _infoTree);
    HRESULT     hr = S_OK;
    CList *     traversalStack = new CList;

    if (!traversalStack)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    gInfo = NULL;
    while ((levelCount < NUM_INFO_LEVELS) || ((thisLevel==NULL) && levelCount > 0))
    {
        if (index[levelCount] == COMPUTE)
        {
            hr = THR( ComputeLevelIndex (ptp, index, levelCount));
        }

        if ( FAILED( hr ) )
            goto Cleanup;

        if (thisLevel == NULL)
        {
            traversalStack->Pop((void**)(&thisLevel));
            levelCount --;
            if (thisLevel == NULL)
            {
                goto Cleanup;
            }
            if (index[levelCount] == s_levelSize[levelCount]-1)
            {
                thisLevel = NULL;
            }
            else 
            {
                index[levelCount] = s_levelSize[levelCount]-1;
            }
        }
        else
        {
            traversalStack->Push ((void**)thisLevel);
            thisLevel = ( *DYNCAST(CTreeList, thisLevel) ) [index[levelCount]];
            if (thisLevel && thisLevel->IsInfoType ())
            {
                gInfo =  DYNCAST(CGlyphInfoType, thisLevel); 
                goto Cleanup;
            }
            levelCount ++;
        }
    }

Cleanup:
    delete traversalStack;
    return hr;
}
            





CGlyph::CList::CList ()
{
    _elemList = NULL;
}

CGlyph::CList::~CList ()
{
    void * dummy;

    while (_elemList != NULL)
    {
        Pop (&dummy);
    }
}


HRESULT 
CGlyph::CList::Push (void * pushed)
{
    ListElemType * newElem = new ListElemType;

    if (newElem == NULL)
        return E_OUTOFMEMORY;

    newElem->elem = pushed;
    newElem->next = _elemList;
    _elemList = newElem;
    return (S_OK);
}

HRESULT 
CGlyph::CList::Pop (void ** popped)
{
    ListElemType * toDelete = _elemList;

    if (_elemList == NULL)
    {
        *popped = NULL;
        goto Cleanup;
    }
    *popped = _elemList->elem;
    _elemList = _elemList->next;
    delete toDelete;

Cleanup:
    return S_OK;
}



CGlyph::CTreeList::CTreeList (LONG numObjects)
{
    int count;

    _numObjects = numObjects;
    _nextLevel = new CTreeObject * [numObjects];
    if (!_nextLevel)
        return;
    for (count = 0; count < numObjects; count ++)
        _nextLevel [count] = NULL;
}
        
CGlyph::CTreeList::~CTreeList () 
{
    int count;

    for (count = 0; count < _numObjects; count ++)
        delete _nextLevel [count];
    delete _nextLevel; 
}

CGlyph::CTreeObject * & 
CGlyph::CTreeList::operator[](int index)
{
    static  CTreeObject * s_pNULL = NULL;
    AssertSz (s_pNULL == NULL, "Invalid Flag Set To NULL");

    if ((index >= 0) && (index < _numObjects))
        return (_nextLevel [index]);
    else
        return s_pNULL;
}


CGlyph::CGlyphInfoType::~CGlyphInfoType () 
{ 
    ReleaseInterface(pImageContext);
    delete pchImgURL;
}

