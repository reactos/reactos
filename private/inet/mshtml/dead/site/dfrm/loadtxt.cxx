//-------------------------------------------------------------------------------------------
// @doc         Internal
// @module filetmp.cpp | implementation file of the <c CLoadText> class
//
// @comm
// Copyright: (c) 1994-1995, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// Contents:
//
//
//
// Functions: None.
//
//      History: created by frankman
//      08/24/94
//      11/16/94    Lajosf  Changed &punk to punk on the parameter list of OleRelease
//      12/02/94    Lajosf  Implement Begin and End COMMENT
//      2/2/95      alexa   Parser rewrite, + change the style of the commnet line
//                          commnetary line starts with ';'. Modified FileLoad logic.
//      02/13/95    jerryd  replace FAKEDNA with DIRT
//
//-------------------------------------------------------------------------------------------

#ifndef PRODUCT_97
#error LoadText should not be built for 96!
#endif

#include "headers.hxx"
#include "dfrm.hxx"

#include <loadtxt.hxx>
#include <fstream.h>


static void AsciiToUnicode(TCHAR *pchUnicode, char *pchAscii, unsigned int cb);
static void UnicodeToAscii(OLECHAR *pchUnicode, char *pchAscii, unsigned int cb);

DeclareTag(tagCLoadText, "CLoadText", "CLoadText class methods")

//-------------------------------------------------------------------------------------------
//
//  GLOBAL VARIABLES
//
//-------------------------------------------------------------------------------------------

static char *g_pchBegin = "BEGIN";
static char *g_pchEnd = "END";
static char *g_pchLayout = "LAYOUT";
static char *g_pchControl = "CONTROL";




/////////////////////////////////////////////////////////////////////////////////
//
//      @mfunc <c CLoadText> Constructor
//      @comm created by frankman
//      09/01/94
//
CLoadText::CLoadText()
{
    SetDefault();
}
/////////////////////////////////////////////////////////////////////////////////







/////////////////////////////////////////////////////////////////////////////////
//
//
//  @mfunc  R|Cl|N|C
//
//  @parm   datatype | parametername | description
//
//  @rdesc  ReturnValue
//
//  @comm   created by frankman
//          02/10/95
//  @comm   Purpose:
//
HRESULT CLoadText::Init(CSite *pRootSite)
{

    Assert(pRootSite);

    _pchLValueToken = new char[k_iLineLen];

    _pDoc = pRootSite->_pDoc;
    _pRootSite = pRootSite;
    return(S_OK);
}
/////////////////////////////////////////////////////////////////////////////////






/////////////////////////////////////////////////////////////////////////////////
//
//      @mfunc <c CLoadText> Destructor
//      @comm created by frankman
//      09/01/94
//
CLoadText::~CLoadText()
{

    if (_pTI)
    {
        _pTI->Release();
    }

    delete _pchLValueToken;
}
/////////////////////////////////////////////////////////////////////////////////






/////////////////////////////////////////////////////////////////////////////////
//
//
//      @mfunc  <c CLoadText> SetDefault, initialising internal state during construction
//
//      @comm   created by frankman
//                      11/04/94
//      @comm   modified by alexa   (added FileName and LineNumber support for debugging purposes).
//                      2/2/95
//      @comm   Purpose:
//
void CLoadText::SetDefault(void)
{
    _iLineNumber = 0;
    _pchToken   = 0;
    _pchLValueToken = 0;
    _pTI   = 0;
}
/////////////////////////////////////////////////////////////////////////////////







//+-----------------------------------------------------------------------------
//
//
//    Member:       CLoadText::FileLoad
//
//    Synopsis:     takes a filename, opens it, and tries to restore a ddoc out of it
//
//    Arguments:
//              TCHAR *pchFileName  - file to open
//
//    Returns:      S_OK            - if all is fine
//                  E_OUTOFMEMORY   - if allocation failed
//                  E_INVALIDARG    - if textfile contains invalid stuff
//
//+-----------------------------------------------------------------------------
HRESULT CLoadText::FileLoad(TCHAR *pchFileName)
{


    HRESULT                 hr = S_OK;
    BOOL                    fWrongFileStructure = FALSE;
    COleDataSite *   pControl = NULL;
    CDataFrame          *   pCurrentDataFrame = NULL;
    CSite         *   pCurrentRoot = _pRootSite;
    Enum_FormType           sectionType;
    CDataFrame          *   pRootTemplate = NULL;
    IDispatch           *   pDisp = NULL;

    *_achFileName = '\0';




    ifstream        file;
    ofstream        logfile;


    PutErrorInfoText(EPART_ACTION, IDS_LOADTEXT);


    _pDoc->_fDeferedPropertyUpdate = TRUE;

    if (pchFileName && *pchFileName)
    {

        char            achBuff[k_iLineLen];    // Input buffer.
        char            achResult[k_iLineLen];  // the result buffer (used for lvalue, control/layout name)
        TCHAR           chUnicode[k_iLineLen];  // buffer to hold unicode string
        TCHAR           chPropValue[k_iLineLen]; // buffer to hold unicode propvalue
        int             iPropIndex;             // property index (in case we parsed property line)
        CRectl          rcl(0,0,0,0);
        CRectl          rclDetail;
        CRectl          rclControl;

        TraceTag((tagCLoadText, "LoadFile"));

        UnicodeToAscii(pchFileName, _achFileName, 512);
        file.open((char*)_achFileName, ios::in | ios::nocreate);
        if (file.fail())
        {
            file.close();
            file.clear();
            char *pFileStart;
            // now check the file name for paths, if no path given, search the file
            if (SearchPath(0,       // searchpath is default
                           pchFileName,  // filename
                           0,       // no extension
                           k_iLineLen/2,    // length of buffer,
                           (TCHAR*)achBuff,   // buffer
                           (TCHAR**)&pFileStart)) // pointer for last backslash in return buffer
            {
                // achbuffer should now contain the complete filename
                // so we will copy and try again

                UnicodeToAscii((TCHAR*)achBuff, _achFileName, 512);
                file.open((char*)_achFileName, ios::in | ios::nocreate);
                if (file.fail())
                {
                    hr = E_INVALIDARG;
                    goto Cleanup;
                }
            }
            else
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }

        logfile.open("ddload.log", ios::out | ios::trunc, filebuf::sh_write | filebuf::sh_read);
        if (logfile.fail())
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        while (!file.eof() && file.good() && SUCCEEDED(hr))
        {
            file.getline((char *) achBuff, k_iLineLen-1);
            _iLineNumber ++;
            _pchInput = achBuff;        // Set the line buffer pointer
            _pchToken = NULL;           // Reset the Token pointer.

            if (file.good())
            {
                // found one more line
                Enum_FormType   eft = ParseSectionType(achResult, &iPropIndex);

                switch (eft)
                {
                    case FormType_Layout:
                        sectionType = eft;

                         hr = pCurrentRoot->InsertDOFromCLSID(
                                CLSID_CDataFrame,
                                &rcl,
                                0,
                                (CSite **) &pCurrentDataFrame);
                        if (hr)
                            goto Cleanup;

                        if (!pRootTemplate)
                        {
                            pRootTemplate = pCurrentDataFrame;
                            pRootTemplate->AddRef();
                        }

                        AsciiToUnicode((TCHAR*)&chUnicode,achResult,k_iLineLen);    // achResult has a NAME of layout
                        pCurrentRoot = pCurrentDataFrame->getDetail ();
                        Assert (pCurrentRoot);

                        ClearInterface(&pDisp); // clear any old cached dispatch

                        // now we should set the name property
                        hr = THR(pCurrentDataFrame->QueryInterface(IID_IDispatch, (void **)&pDisp));
                        if (hr)
                            goto Cleanup;

                        SetPropertyFromText(
                                pDisp,
                                _T("Name"),
                                chUnicode,
                                TRUE);
                        rclDetail = pCurrentRoot->_rcl;
                        break;

                    case FormType_Control:
                        sectionType = eft;

                        if (pCurrentDataFrame && pCurrentRoot)
                        {
                            RECTL rcl = {0, 0, 0, 0};
                            hr = pCurrentRoot->InsertDOFromCLSID(
                                    CLSID_CMdcText,
                                    &rcl,
                                    0,
                                    (CSite **) &pControl);
                            if (!hr)
                            {
                                IControlElement * pControlI;

                                hr = pControl->GetControlI(&pControlI);
                                if (hr)
                                    goto Error;
                                AsciiToUnicode((TCHAR*)&chUnicode,achResult,k_iLineLen);    // achResult has a NAME of control
                                SetPropertyFromText(pControlI, _T("Name"), chUnicode, TRUE);
                                pControlI->Release();
                            }
                            else
                                goto MemoryError;
                        }
                        else
                        {
                            // there must be a parent layout, otherwise error in
                            // text file.
                            hr = E_INVALIDARG;
                        }
                        break;

                    case FormType_Property:
                        // convert the ascii tokens to unicode
                        AsciiToUnicode((TCHAR*)&chUnicode,_pchLValueToken,k_iLineLen);    // propname
                        AsciiToUnicode((TCHAR*)&chPropValue,_pchToken,k_iLineLen);    // propvalue
                        if (sectionType == FormType_Layout)
                        {
                            hr = SetPropertyFromText(
                                            pDisp,
                                            chUnicode,
                                            chPropValue);
                        }
                        else
                        {
                            IControlElement * pControlI;

                            hr = pControl->GetControlI(&pControlI);
                            if (hr)
                                goto Error;
                            Assert (sectionType == FormType_Control);
                            hr = SetPropertyFromText(pControlI, chUnicode, chPropValue);
                            pControlI->Release();
                        }


                        if (FAILED(hr))
                        {
                            // save the current property name to a log file, so that we can look it up later
                            logfile << "Unknown property ";
                            if (sectionType == FormType_Layout)
                                logfile << "on layout:   ";
                            else
                                logfile << "on control:  ";
                            logfile << _pchLValueToken;
                            //logfile << endl;
                            logfile << '\n' << logfile.flush();
                            hr = S_OK;  // we want to continue, breaking just because of text errors is cruel
                            fWrongFileStructure = TRUE;
                        }
                        break;

                    case FormType_EndLayoutSection:
                        Assert (sectionType == FormType_Layout);
                        pCurrentRoot->Move(rclDetail.left, rclDetail.top,
                                           rclDetail.Width(), rclDetail.Height());
                        pCurrentRoot = (CBaseFrame *)pCurrentDataFrame->_pParent;
                        break;

                    case FormType_EndControlSection:
                        if (pControl->_fProposedSet)
                        {
                            pControl->GetProposed(pControl, &rclControl);
                        }
                        else
                        {
                            rclControl = pControl->_rcl;
                        }
                        rclDetail.UnionRect(&rclDetail, &rclControl);
                        Assert (sectionType == FormType_Control);
                        sectionType = FormType_Layout;
                        break;

                    case FormType_Comment:
                        break;

                    case FormType_Error:
                        hr = E_INVALIDARG;
                        break;

                }   // end of (switch on type of parsed section)

            }   // end of (if read of the line from file succeded)

        }   // end of (while reading file loop).

    }   // end of (if file name is correct).

    if (SUCCEEDED(hr))
    {
        // first set up the header/footer combinations
        hr = SetHeaderFooterRelations();
        if (SUCCEEDED(hr))
        {
            // now set up the database stuff....
            hr = pRootTemplate->SetDeferedPropertyUpdate(FALSE);
            // hr = pRootTemplate->SetDatabaseRelations();
        }
    }
    else
        goto Error;

Cleanup:

    ClearInterface(&pDisp); // clear any old cached dispatch
    _pDoc->_fDeferedPropertyUpdate = FALSE;
    if (SUCCEEDED(hr) && fWrongFileStructure)
    {
        // should return error using great error routines

        PutErrorInfoText(EPART_ERROR, IDS_LOADTEXTUNKNOWNPROP);
    }

    if (pRootTemplate)
    {
        pRootTemplate->Release();
    }

    file.close();
    logfile.close();

    RRETURN (hr);

Error:

    if (pRootTemplate)
    {
        pRootTemplate->_pParent->DeleteSite(pRootTemplate, 0);
    }
    goto Cleanup;

MemoryError:
    hr  = E_OUTOFMEMORY;
    goto Cleanup;

}
//+-End Of Method---------------------------------------------------------------






//+-----------------------------------------------------------------------------
//
//
//    Member:       CLoadText::SetHeaderFooterRelations
//
//    Synopsis:     walks the template tree and sets up header-footer relationships
//
//    Arguments:    none
//
//    Returns:      HRESULT S_OK if everything is fine, E_FAIL indicates critical stop condition
//
//-----------------------------------------------------------------------------
HRESULT
CLoadText::SetHeaderFooterRelations(void)
{
    HRESULT hr = S_OK;

    RRETURN(hr);
}
//+-End Of Method---------------------------------------------------------------















/////////////////////////////////////////////////////////////////////////////////
//
//
//      @mfunc  saves the current state of templates into a text file
//
//      @rdesc  ReturnValue
//
//      @comm   created by frankman
//                      08/25/94
//      @comm   Purpose:
HRESULT CLoadText::FileSave(OLECHAR *pchFileName)
{

    return (E_NOTIMPL);
}
/////////////////////////////////////////////////////////////////////////////////




























//+---------------------------------------------------------------------------
//
//      P A R S E R
//




//+---------------------------------------------------------------------------
//
//  @mfunc  SkipBlanks member function of the <c Class>.  Skips the blanks, comments
//          in the current buffer (one line of text).
//
//  @rdesc  void
//  @comm   effects: modifies the pointer to an input line.
//
//  @comm   created by alexa
//          02/03/95
//  @comm   Purpose: To skip blanks, empty lines, comments
//  @comm   The following can be optimized more, but for now it just have to do a job.
//
//----------------------------------------------------------------------------

#define IsSeparator(c)  (c == ' ' || c == '\t' || c == cSeparator)

void CLoadText::SkipBlanks (char cSeparator)
{
    char *p = _pchInput;
    Assert (_pchInput);

    char c;

    while ((c = *p) != '\0')
    {
        if (IsSeparator (c))
        {
            p++;
        } else  if (c == ';')   // comment line
            {
                p = NULL;
                break;
            } else
            {
                // any other letter
                break;
            }
    }

    _pchInput = p;      // Side effect.

    return;
}




//+---------------------------------------------------------------------------
//
//  @mfunc  GetToken member function of the <c CLoadText>. Gets the next token in line.
//
//  @parm   char | cSeparator | separator symbol (default is '\0').
//
//  @comm   created by alexa
//          02/03/95
//  @comm   Purpose:
//
//----------------------------------------------------------------------------

void CLoadText::GetToken (char cSeparator)
{
    _pchToken = NULL;

    if (_pchInput)
    {
        SkipBlanks (cSeparator);
        if (IsLineNotEmpty())
        {
            _pchToken = _pchInput;
            while (!IsSeparator(*_pchInput))
            {
                _pchInput ++;
            }
            *_pchInput = '\0';          // we have to make shure that we have always an extra '\0' byte after the input buffer.
            _pchInput ++;
        }
    }
    return;
}




//+---------------------------------------------------------------------------
//
//  @mfunc  GetStringToken member function of the <c CLoadText>. Gets the next string token in line.
//
//  @comm   created by alexa
//          02/03/95
//  @comm   Purpose:
//
//----------------------------------------------------------------------------

#define StringSeparator ('"')
void CLoadText::GetStringToken ()
{
    _pchToken = NULL;

    if (_pchInput)
    {
        SkipBlanks (StringSeparator);       // pass separator
        if (IsLineNotEmpty())               // if line is not empty
        {
            _pchToken = _pchInput;
            char c;
            while ((c = *_pchInput) != '\0' && c != StringSeparator)
            {
                _pchInput ++;
            }
            *_pchInput = '\0';          // we have to make shure that we have always an extra '\0' byte after the input buffer.
            _pchInput ++;
        }
    }
    return;
}





//+---------------------------------------------------------------------------
//
//
//      @mfunc  simple one line parser, returning indication for section type
//
//      @rdesc  one of the FormType_XXXX enums, puts section name in pchName
//
//      @comm   created by frankman
//                      08/26/94
//      @comm   modified by alexa  (implemented simple recursive descent parser)
//                      2/2/95
//
//----------------------------------------------------------------------------


//+---------------------------------------------------------------
//
//  Member:     CLoadText::ParseSectionType
//
//  Synopsis:   parses the current line in the file
//
//  Arguments:  none
//
//  Returns:    the section type, can be any of the FormType_XXX enums
//
//---------------------------------------------------------------
Enum_FormType CLoadText::ParseSectionType (char *pResult, int *piPropIndex)
{

    Assert(_pchInput);
    Assert(pResult);
    Assert(piPropIndex);

    Enum_FormType   eft = FormType_Unknown;

    if (IsLineNotEmpty ())
    {
        _strupr(_pchInput);                 // Capitilize the line
        GetToken (':');

        if (IsTokenFound())
        {

            // Parse for "Begin" or "End" key words
            if (IsTokenMatches (g_pchBegin))
            {
                // Parse for the type of the section "Layout" or "Control"
                GetToken ();
                if (IsTokenMatches(g_pchLayout))
                {
                    eft = FormType_Layout;
                    AcceptToken ();
                    eft = ParseName (eft, pResult);
                }
                else if (IsTokenMatches(g_pchControl))
                {
                    eft = FormType_Control;
                    AcceptToken ();
                    eft = ParseName (eft, pResult);
                };

            }
            else if (IsTokenMatches (g_pchEnd))
            {
                // Parse for the type of the section "Layout" or "Control"
                GetToken ();
                if (IsTokenMatches(g_pchLayout))
                {
                    eft = FormType_EndLayoutSection;
                    AcceptToken ();
                }
                else if (IsTokenMatches(g_pchControl))
                {
                    eft = FormType_EndControlSection;
                    AcceptToken ();
                };
            }
            else
            {
                // else it's a property or something else
                eft = FormType_Property;
                // this now puts the property name in _pchLValueToken and the value in _pchToken
                AcceptToken ();
                GetStringToken();
            }
        }
        else
        {
            // if token is not found in the line (the line is a comment).
            // it's a comment
            eft = FormType_Comment;
        }

    } // end of check for IsLineNotEmpty ()
    return eft;
}





//+---------------------------------------------------------------------------
//
//  @mfunc  scan the name of the section.
//
//  @param  Enum_FormType | eft | layout/control type
//  @param  char * | pchName | pointer to a name buffer
//
//  @comm   careted by alexa
//          2/3/95
//
//----------------------------------------------------------------------------

Enum_FormType CLoadText::ParseName (Enum_FormType eft, char *pchName)
{


    Assert (pchName);

    // Scan for  the name token.
    GetToken ();

    if (!IsTokenFound())
    {
        Assert(FALSE);
        _pDoc->ShowMessage(NULL, MB_ERRORFLAGS, 0,
                IDS_SYNTAX_ERROR,
                GetFileName(),
                GetLineNumber(),
                "section NAME is missing");
        *pchName = '\0';
        eft = FormType_Error;
    } else
    {
        strcpy(pchName, _pchToken);         // copy the name token into a name buffer.
        AcceptToken ();
    }
    return eft;
}










//
//      E N D   O F   P A R S E R
//
//----------------------------------------------------------------------------








//+-----------------------------------------------------------------------------
//
//
//    Function:    AsciiToUnicode
//
//    Synopsis:    takes an ASCII string and converts it to unicode
//
//    Arguments:
//              TCHAR *pchUnicode   - caller allocated unicode buffer
//              char  *pchAscii     - caller allocated ascii text
//              unsigned int cb     - length of unicode buffer
//
//    Returns:      nothing
//
//+-----------------------------------------------------------------------------
static void AsciiToUnicode(TCHAR *pchUnicode, char *pchAscii, unsigned int cb)
{
    Assert(pchUnicode);

    if (pchAscii)
    {
        TCHAR    wUniChar = 0;
        unsigned int uiLen = strlen(pchAscii);

        Assert(uiLen <= cb);

        for (unsigned int uiCounter=0; uiCounter < uiLen; uiCounter++)
        {
            wUniChar = *pchAscii;
            *pchUnicode = wUniChar;
            pchUnicode++;
            pchAscii++;
        }
    }
    *pchUnicode = '\0';
}
//+-End Of Method---------------------------------------------------------------







//+-----------------------------------------------------------------------------
//
//
//    Function:    UnicodeToAscii
//
//    Synopsis:    takes an Unicode string and converts it to Ascii
//
//    Arguments:
//              TCHAR *pchUnicode   - caller allocated unicode buffer
//              char  *pchAscii     - caller allocated ascii text
//              unsigned int cb     - length of Ascii buffer
//
//    Returns:      nothing
//
//+-----------------------------------------------------------------------------
static void UnicodeToAscii(TCHAR *pchUnicode, char *pchAscii, unsigned int cb)
{
    Assert(pchAscii);

    if (pchUnicode)
    {
        unsigned int uiLen = _tcslen(pchUnicode);

        Assert(uiLen <= cb);

        for (unsigned int uiCounter=0; uiCounter < uiLen; uiCounter++)
        {
            *pchAscii = (char) LOBYTE(*pchUnicode);
            pchUnicode++;
            pchAscii++;
        }
    }
    *pchAscii = '\0';
}
//+-End Of Method---------------------------------------------------------------



HRESULT
CLoadText::SetPropertyFromText(IDispatch * pDisp, TCHAR * pchPropName, TCHAR *pchPropValue, BOOL fNewTypeLib)
{
    HRESULT hr = E_FAIL;

    Assert(pDisp);
    Assert(pchPropName);
    Assert(pchPropValue);


    if (fNewTypeLib || !_pTI)
    {
        ITypeInfo *pti=0;
        if (_pTI)
        {
            pti = _pTI;
            _pTI = 0;
        }
        hr = pDisp->GetTypeInfo(0, LOCALE_SYSTEM_DEFAULT, &_pTI);
        if (FAILED(hr))
            goto Cleanup;

        if (pti)
        {
            pti->Release();
        }
    }

    Assert(_pTI);
    DISPID dispid;

    hr = _pTI->GetIDsOfNames(&pchPropName, 1, &dispid);

    if (SUCCEEDED(hr))
    {
        // we found the property inside
        //  our tables, so let's try to set it
        VARIANT     variant;
        EXCEPINFO   except;
        BSTR        bstr;

        InitEXCEPINFO(&except);

        FormsAllocString(pchPropValue, &bstr);
        VariantInit(&variant);
        // set up the variant
        CVarToVARIANTARG(&bstr, VT_BSTR, &variant);

        hr = SetDispProp(pDisp, dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, &variant, &except);

        FormsFreeString(bstr);
        FreeEXCEPINFO(&except);
    }
Cleanup:
    RRETURN(hr);
}
//+-End Of Method---------------------------------------------------------------





