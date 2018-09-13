
//-------------------------------------------------------------------------------------------
// @doc     Internal
// @module FILETMP.H | shortdescription
//
// @comm
// Copyright: (c) 1994-1995, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// @comm Contents:
//
//
//
// Functions: None.
//
//  History: created by frankman
//  08/30/94
//  History: modified by alexa (added parsing support, tokenizer, syntax error handling, etc.)
//  2/2/95
//  History: modified by jerryd (integrated DIRT OLE DB provider)
//  2/25/95
//
//-------------------------------------------------------------------------------------------
//
#ifndef     __FILETMP__INCL__
#   define  __FILETMP__INCL__   1

const   int k_iLineLen=1025;

typedef enum
{
    FormType_Layout,
    FormType_Control,
    FormType_Unknown,
    FormType_EndLayoutSection,
    FormType_EndControlSection,
    FormType_Comment,
    FormType_Property,
    FormType_Error,
}   Enum_FormType;



/////////////////////////////////////////////////////////////////////////////////////////////
//
//  @class subclass for initial try, some added fakes
//
//  @comm Purpose:
//      for the first run this subclass includes some faking functionallity
//
//  @comm created by frankman
//      08/25/94
//  @comm modified by alexa (added file parser/tokenizer)
//      2/2/95
//  @comm Hungarian:
//
class CLoadText
{
public:
    // constructor, destructor, operators go here
    // @access public
    CLoadText();
    virtual ~CLoadText();

public:
    HRESULT         Init(CSite *pRootSite);
    HRESULT         FileLoad(OLECHAR *pchFileName);     // @member FileLoad | creates templates from a given descriptionfile
    HRESULT         FileSave(OLECHAR *pchFileName);     // @member FileSave | saves the existing templates into an ASCII file

protected:
    // @access protected
    Enum_FormType   ParseSectionType(char * pResult, int *piPropIndex); // @member ParseSectionType | simple parsing function
    HRESULT         SetHeaderFooterRelations(void);
    HRESULT         SetPropertyFromText(IDispatch *pDisp, TCHAR *pchPropName, TCHAR *pchPropValue, BOOL fNewTypeLib=FALSE);


    int     GetLineNumber ()                            // Get the current file line number.
        { return _iLineNumber; }
    char *  GetFileName ()                              // Get the file name.
        { return _achFileName; }

private:
    void GetToken   (char cSeparator = '\0');           // Get the next token in line.

    void GetStringToken ();                             // "...."

    void SkipBlanks (char cSeparator);                  // Skip blanks and separators.

    inline BOOL IsTokenMatches (char *pMatchingStr)     // Check if current token matches the string.
        { return (strcmp (_pchToken, pMatchingStr) == 0); }

    inline BOOL IsTokenFound ()                         // Check if there are token in the line (have to be called after GetToken).
        { return _pchToken && *_pchToken; }

    inline void AcceptToken ()                          // Accept the token as being parsed.
        { if (_pchToken && _pchLValueToken) strcpy(_pchLValueToken, _pchToken); _pchToken = NULL; }



    Enum_FormType ParseName (Enum_FormType eft, char *pchName); // Scan the name of the layout/control.

    inline BOOL IsLineNotEmpty ()                       // Check if the input line is not empty.
        {return _pchInput && *_pchInput; }

protected:
    // data
    char            _achFileName[512];                  // holds the filename
    int             _iLineNumber;                       // Get the parsed file line number.
    char           *_pchInput;                          // Cached pointer to a line buffer.
    char           *_pchToken;                          // Cashed last scanned token.
    char           *_pchLValueToken;                    // cashed previous token, is either the section or prop name
    CDoc           *_pDoc;
    CSite          *_pRootSite;
    TCHAR           _wcharMungedQuery[512];
    ITypeInfo       * _pTI;

private:
    // @access private
    void SetDefault(void);


};
/////////////////////////////////////////////////////////////////////////////////////////////



#endif  // __FILETMP__INCL__
