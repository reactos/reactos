//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// xmlutil.h 
//
//   XML item helper functions.
//
//   History:
//
//       4/1/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////


//
// Check for previous includes of this file.
//

#ifndef _XMLUTIL_H_

#define _XMLUTIL_H_

//
// Attribute enumeration.  Contains the attributes supported by
// the XML_GetAttribute function.
//

typedef enum _tagXML_ATTRIBUTE
{
    XML_TITLE           = 0,
    XML_TITLE_ATTR      = 1,
    XML_HREF            = 2,
    XML_ABSTRACT        = 3,
    XML_ABSTRACT_ATTR   = 4,
    XML_ICON            = 5,
    XML_LOGO            = 6,
    XML_LOGO_DEFAULT    = 7,
    XML_SELF            = 8,
    XML_SELF_OLD        = 9,
    XML_BASE            = 10,
    XML_USAGE           = 11,
    XML_USAGE_CHANNEL   = 12,
    XML_USAGE_DSKCMP    = 13,
    XML_WIDTH           = 14,
    XML_HEIGHT          = 15,
    XML_CANRESIZE       = 16,
    XML_CANRESIZEX      = 17,
    XML_CANRESIZEY      = 18,
    XML_PREFERREDLEFT   = 19,
    XML_PREFERREDTOP    = 20,
    XML_OPENAS          = 21,
    XML_SHOW            = 22,
    XML_SHOW_CHANNEL    = 23,
    XML_SHOW_DSKCMP     = 24,
    XML_A_HREF          = 25,
    XML_INFOURI         = 26,
    XML_LOGO_WIDE       = 27,
    XML_LOGIN           = 28,
    XML_USAGE_SOFTWAREUPDATE = 29,
    XML_SHOW_SOFTWAREUPDATE  = 30,
    XML_ITEMSTATE       = 31,
    XML_NULL            = 99
} XML_ATTRIBUTE;

//
// XML document types.
//

typedef enum _tagXMLDOCTYPE {
    DOC_CHANNEL,
    DOC_DESKTOPCOMPONENT,
    DOC_SOFTWAREUPDATE,
    DOC_UNKNOWN
} XMLDOCTYPE;



//
// Cdf string constants used in XML files.
//

#define WSTR_EMPTY          L""

// Elements
#define WSTR_A              L"A"
#define WSTR_ABSTRACT       L"ABSTRACT"
#define WSTR_RESIZE         L"CANRESIZE"
#define WSTR_RESIZEX        L"CANRESIZEX"
#define WSTR_RESIZEY        L"CANRESIZEY"
#define WSTR_CHANNEL        L"CHANNEL"
#define WSTR_DSKCMP         L"DESKTOPCOMPONENT"
#define WSTR_HEIGHT         L"HEIGHT"
#define WSTR_ITEM           L"ITEM"
#define WSTR_LOGIN          L"LOGIN"
#define WSTR_LOGO           L"LOGO"
#define WSTR_OPENAS         L"OPENAS"
#define WSTR_PREFLEFT       L"PREFERREDLEFT"
#define WSTR_PREFTOP        L"PREFERREDTOP"
#define WSTR_SCRNSAVE       L"SCREENSAVER"
#define WSTR_SELF           L"SELF"
#define WSTR_SHOW           L"SHOW"
#define WSTR_SMARTSCRN      L"SMARTSCREEN"
#define WSTR_SOFTDIST       L"SOFTPKG"
#define WSTR_TITLE          L"TITLE"
#define WSTR_USAGE          L"USAGE"
#define WSTR_WIDTH          L"WIDTH"
#define WSTR_SOFTWAREUPDATE L"SOFTWAREUPDATE"
#define WSTR_ITEMSTATE      L"ITEMSTATE"

// Attributes
#define WSTR_BASE           L"BASE"
#define WSTR_HREF           L"HREF"
#define WSTR_INFOURI        L"INFOURI"
#define WSTR_STYLE          L"STYLE"
#define WSTR_VALUE          L"VALUE"

// Attribute values.
#define WSTR_ICON           L"ICON"
#define WSTR_IMAGE          L"IMAGE"
#define WSTR_IMAGEW         L"IMAGE-WIDE"
#define WSTR_ZERO           L"0"
#define WSTR_YES            L"YES"
#define WSTR_HTML           L"HTML"
#define WSTR_NORMAL         L"NORMAL"
#define WSTR_SPLITSCREEN    L"SPLITSCREEN"
#define WSTR_FULLSCREEN     L"FULLSCREEN"


//
// Function protoypes.
//

HRESULT     XML_SynchronousParse(IXMLDocument* pIXMLDocument,
                                 LPTSTR szPath);

HRESULT     XML_DownloadLogo(IXMLDocument* pIXMLDocument);
HRESULT     XML_DownloadImages(IXMLDocument* pIXMLDocument);
HRESULT     XML_RecursiveImageDownload(IXMLElement* pIXMLElement);
HRESULT     XML_DownloadImage(LPCWSTR pwszURL);

XMLDOCTYPE  XML_GetDocType(IXMLDocument* pIXMLDocument);



HRESULT     XML_GetChildElementCollection(
                            IXMLElementCollection *pParentIXMLElementCollection,
                            LONG nIndex,
                            IXMLElementCollection** ppIXMLElementCollection);

HRESULT     XML_GetFirstChannelElement(IXMLDocument* pIXMLDocument,
                                       IXMLElement** ppIXMLElement,
                                       PLONG pnIndex);

HRESULT     XML_GetDesktopElementFromChannelElement(
                                        IXMLElement* pChannelIXMLElement,
                                        IXMLElement** ppIXMLElement,
                                        PLONG pnIndex);

HRESULT     XML_GetFirstDesktopComponentElement(IXMLDocument* pIXMLDocument,
                                                IXMLElement** ppIXMLElement,
                                                PLONG pnIndex);

HRESULT     XML_GetFirstDesktopComponentUsageElement(
                                                   IXMLDocument* pIXMLDocument,
                                                   IXMLElement** ppIXMLElement);

HRESULT     XML_GetDesktopComponentInfo(IXMLDocument* pIXMLDocument,
                                        COMPONENT* pInfo);


HRESULT     XML_GetElementByIndex(IXMLElementCollection* pIXMLElementCollection,
                       LONG nIndex,
                       IXMLElement** ppIXMLElement);

HRESULT     XML_GetElementByName(IXMLElementCollection* pIXMLElementCollection,
                       LPWSTR szNameW,
                       IXMLElement** ppIXMLElement);

BSTR        XML_GetAttribute(IXMLElement *pIXMLElement,
                             XML_ATTRIBUTE attribute);

BSTR        XML_GetChildAttribute(IXMLElement *pIXMLElement,
                                  LPWSTR szChildW,
                                  LPWSTR szAttributeW,
                                  LPWSTR szQualifierW,
                                  LPWSTR szQualifierValueW);

BSTR        XML_GetElementAttribute(IXMLElement *pIXMLElement,
                                    LPWSTR szAttributeW,
                                    LPWSTR szQualifierW,
                                    LPWSTR szQualifierValueW);

HRESULT     XML_GetSubscriptionInfo(IXMLElement* pIXMLElement,
                                    SUBSCRIPTIONINFO* psi);

HRESULT     XML_GetScreenSaverElement(IXMLElement* pIXMLElement,
                                      IXMLElement** ppScreenSaverElement);

BSTR        XML_GetBaseURL(IXMLElement* pIXMLElement);
BSTR        XML_CombineURL(BSTR bstrBaseURL, BSTR bstrRelURL);
BOOL        XML_IsCdfDisplayable(IXMLElement* pIXMLElement);
BOOL        XML_IsSoftDistDisplayable(IXMLElement* pIXMLEelement);
BOOL        XML_IsUsageChannel(IXMLElement* pIXMLElement);
BOOL        XML_IsScreenSaver(IXMLElement* pIXMLElement);
BOOL        XML_IsFolder(IXMLElement* pIXMLElement);
BOOL        XML_ContainsFolder(IXMLElementCollection* pIXMLElementCollection);

BOOL        XML_ChildContainsFolder(
                            IXMLElementCollection* pIXMLElementCollectionParent,
                            ULONG nIndexChild);

BOOL        XML_IsChannel(IXMLElement* pIXMLElement);
BOOL        XML_IsDesktopComponent(IXMLElement* pIXMLElement);
BOOL        XML_IsDesktopComponentUsage(IXMLElement* pIXMLElement);

BOOL        XML_IsCdfidlMemberOf(IXMLElementCollection* pIXMLElementCollection,
                                 PCDFITEMIDLIST pcdfidl);

BSTR        XML_GetGrandChildContent(IXMLElement* pIXMLElement,
                                 LPWSTR szChildW);


BOOL inline XML_IsStrEqualW(LPWSTR p1, LPWSTR p2);

HRESULT XML_MarkCacheEntrySticky(LPTSTR lpszURL);

HRESULT     XML_GetScreenSaverURL(IXMLDocument* pXMLDocument, BSTR* pbstrSSURL);

#endif // _XMLUTIL_H_

