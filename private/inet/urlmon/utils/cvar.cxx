#include <malloc.h>
#include <urlint.h>
#include <map_kv.h>
#include "coll.hxx"
#ifndef unix
#include "COleTime.hxx"
#else
#include "coletime.hxx"
#endif /* unix */
#include "cvar.hxx"

//void * _cdecl operator new(size_t sizeEl, ULONG cEl);
void * _cdecl operator new(size_t sizeEl, COleVariant *pCVar);

void * _cdecl operator new(size_t sizeEl, COleVariant *pCVar)
{
    return  0;
}

/*
struct AFX_EXCEPTION_LINK
{
#ifdef _AFX_OLD_EXCEPTIONS
        union
        {
                _AFX_JUMPBUF m_jumpBuf;
                struct
                {
                        void (PASCAL* pfnCleanup)(AFX_EXCEPTION_LINK* pLink);
                        void* pvData;       // extra data follows
                } m_callback;       // callback for cleanup (nType != 0)
        };
        UINT m_nType;               // 0 for setjmp, !=0 for user extension
#endif //!_AFX_OLD_EXCEPTIONS

        AFX_EXCEPTION_LINK* m_pLinkPrev;    // previous top, next in handler chain
        CException* m_pException;   // current exception (NULL in TRY block)

        AFX_EXCEPTION_LINK();       // for initialization and linking
        ~AFX_EXCEPTION_LINK()       // for cleanup and unlinking
                { AfxTryCleanup(); };
};
*/
#define TRY { AFX_EXCEPTION_LINK _afxExceptionLink; try {

#define CATCH(class, e) } catch (class* e) \
        { ASSERT(e->IsKindOf(RUNTIME_CLASS(class))); \
                _afxExceptionLink.m_pException = e;

#define AND_CATCH(class, e) } catch (class* e) \
        { ASSERT(e->IsKindOf(RUNTIME_CLASS(class))); \
                _afxExceptionLink.m_pException = e;

#define END_CATCH } }


#define THROW(e) throw e
#define THROW_LAST() (AfxThrowLastCleanup(), throw)

// Advanced macros for smaller code
#define CATCH_ALL(e) } catch (CException* e) \
        { { ASSERT(e->IsKindOf(RUNTIME_CLASS(CException))); \
                _afxExceptionLink.m_pException = e;

#define AND_CATCH_ALL(e) } catch (CException* e) \
        { { ASSERT(e->IsKindOf(RUNTIME_CLASS(CException))); \
                _afxExceptionLink.m_pException = e;

#define END_CATCH_ALL } } }

#define END_TRY } catch (CException* e) \
        { ASSERT(e->IsKindOf(RUNTIME_CLASS(CException))); \
                _afxExceptionLink.m_pException = e; } }

#define AFX_OLE_FALSE 0
#define AFX_OLE_TRUE (-1)

#ifndef _DEBUG
#define USES_CONVERSION int _convert; _convert
#else
#define USES_CONVERSION int _convert = 0;
#endif

#ifdef _DEBUG
#define UNUSED(x)
#else
#define UNUSED(x) x
#endif
#define UNUSED_ALWAYS(x) x


/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers

LPWSTR AFXAPI AfxA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars);
LPSTR AFXAPI AfxW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars);

#define A2CW(lpa) (\
        ((LPCSTR)lpa == NULL) ? NULL : (\
                _convert = (lstrlenA(lpa)+1),\
                (LPCWSTR)AfxA2WHelper((LPWSTR) alloca(_convert*2), lpa, _convert)\
        )\
)

#define A2W(lpa) (\
        ((LPCSTR)lpa == NULL) ? NULL : (\
                _convert = (lstrlenA(lpa)+1),\
                AfxA2WHelper((LPWSTR) alloca(_convert*2), lpa, _convert)\
        )\
)

#define W2CA(lpw) (\
        ((LPCWSTR)lpw == NULL) ? NULL : (\
                _convert = (wcslen(lpw)+1)*2,\
                (LPCSTR)AfxW2AHelper((LPSTR) alloca(_convert), lpw, _convert)\
        )\
)

#define W2A(lpw) (\
        ((LPCWSTR)lpw == NULL) ? NULL : (\
                _convert = (wcslen(lpw)+1)*2,\
                AfxW2AHelper((LPSTR) alloca(_convert), lpw, _convert)\
        )\
)


#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
        #define DEVMODEOLE DEVMODEW
        #define LPDEVMODEOLE LPDEVMODEW
        #define TEXTMETRICOLE TEXTMETRICW
        #define LPTEXTMETRICOLE LPTEXTMETRICW
        inline size_t ocslen(LPCOLESTR x) { return wcslen(x); }
        inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) { return wcscpy(dest, src); }
        inline LPCOLESTR T2COLE(LPCTSTR lp) { return lp; }
        inline LPCTSTR OLE2CT(LPCOLESTR lp) { return lp; }
        inline LPOLESTR T2OLE(LPTSTR lp) { return lp; }
        inline LPTSTR OLE2T(LPOLESTR lp) { return lp; }
        inline LPOLESTR TASKSTRINGT2OLE(LPOLESTR lp) { return lp; }
        inline LPTSTR TASKSTRINGOLE2T(LPOLESTR lp) { return lp; }
        inline LPDEVMODEW DEVMODEOLE2T(LPDEVMODEOLE lp) { return lp; }
        inline LPDEVMODEOLE DEVMODET2OLE(LPDEVMODEW lp) { return lp; }
        inline LPTEXTMETRICW TEXTMETRICOLE2T(LPTEXTMETRICOLE lp) { return lp; }
        inline LPTEXTMETRICOLE TEXTMETRICT2OLE(LPTEXTMETRICW lp) { return lp; }
        inline BSTR BSTR2TBSTR(BSTR bstr) { return bstr;}
#elif defined(OLE2ANSI)
// in these cases the default (TCHAR) is the same as OLECHAR
        #define DEVMODEOLE DEVMODEA
        #define LPDEVMODEOLE LPDEVMODEA
        #define TEXTMETRICOLE TEXTMETRICA
        #define LPTEXTMETRICOLE LPTEXTMETRICA
        inline size_t ocslen(LPCOLESTR x) { return lstrlenA(x); }
        inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) { return lstrcpyA(dest, src); }
        inline LPCOLESTR T2COLE(LPCTSTR lp) { return lp; }
        inline LPCTSTR OLE2CT(LPCOLESTR lp) { return lp; }
        inline LPOLESTR T2OLE(LPTSTR lp) { return lp; }
        inline LPTSTR OLE2T(LPOLESTR lp) { return lp; }
        inline LPOLESTR TASKSTRINGT2OLE(LPOLESTR lp) { return lp; }
        inline LPTSTR TASKSTRINGOLE2T(LPOLESTR lp) { return lp; }
        inline LPDEVMODE DEVMODEOLE2T(LPDEVMODEOLE lp) { return lp; }
        inline LPDEVMODEOLE DEVMODET2OLE(LPDEVMODE lp) { return lp; }
        inline LPTEXTMETRIC TEXTMETRICOLE2T(LPTEXTMETRICOLE lp) { return lp; }
        inline LPTEXTMETRICOLE TEXTMETRICT2OLE(LPTEXTMETRIC lp) { return lp; }
        inline BSTR BSTR2TBSTR(BSTR bstr) { return bstr; }
#else
        #define DEVMODEOLE DEVMODEW
        #define LPDEVMODEOLE LPDEVMODEW
        #define TEXTMETRICOLE TEXTMETRICW
        #define LPTEXTMETRICOLE LPTEXTMETRICW
        inline size_t ocslen(LPCOLESTR x) { return wcslen(x); }
        inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) { return wcscpy(dest, src); }
        #define T2COLE(lpa) A2CW(lpa)
        #define T2OLE(lpa) A2W(lpa)
        #define OLE2CT(lpo) W2CA(lpo)
        #define OLE2T(lpo) W2A(lpo)
        #define TASKSTRINGT2OLE(lpa)    AfxTaskStringA2W(lpa)
        #define TASKSTRINGOLE2T(lpo) AfxTaskStringW2A(lpo)
        #define DEVMODEOLE2T(lpo) DEVMODEW2A(lpo)
        #define DEVMODET2OLE(lpa) DEVMODEA2W(lpa)
        #define TEXTMETRICOLE2T(lptmw) TEXTMETRICW2A(lptmw)
        #define TEXTMETRICT2OLE(lptma) TEXTMETRICA2W(lptma)
        #define BSTR2TBSTR(bstr) AfxBSTR2ABSTR(bstr)
#endif

#ifdef OLE2ANSI
        #define W2OLE W2A
        #define W2COLE W2CA
        #define OLE2W A2W
        #define OLE2CW A2CW
        inline LPOLESTR A2OLE(LPSTR lp) { return lp; }
        inline LPCOLESTR A2COLE(LPCSTR lp) { return lp; }
        inline LPSTR OLE2A(LPOLESTR lp) { return lp; }
        inline LPCSTR OLE2CA(LPCOLESTR lp) { return lp; }
#else
        #define A2OLE A2W
        #define A2COLE A2CW
        #define OLE2A W2A
        #define OLE2CA W2CA
        inline LPOLESTR W2OLE(LPWSTR lp) { return lp; }
        inline LPCOLESTR W2COLE(LPCWSTR lp) { return lp; }
        inline LPWSTR OLE2W(LPOLESTR lp) { return lp; }
        inline LPCWSTR OLE2CW(LPCOLESTR lp) { return lp; }
#endif

/////////////////////////////////////////////////////////////////////////////
// helpers

static void PASCAL CheckError(SCODE sc);
static BOOL PASCAL CompareSafeArrays(SAFEARRAY* parray1, SAFEARRAY* parray2);
static void PASCAL CreateOneDimArray(VARIANT& varSrc, DWORD dwSize);
static void PASCAL CopyBinaryData(SAFEARRAY* parray,
        const void* pSrc, DWORD dwSize);

/////////////////////////////////////////////////////////////////////////////
// COleVariant class

COleVariant::COleVariant(const VARIANT& varSrc)
{
        AfxVariantInit(this);
        CheckError(::VariantCopy(this, (LPVARIANT)&varSrc));
}

COleVariant::COleVariant(LPCVARIANT pSrc)
{
        AfxVariantInit(this);
        CheckError(::VariantCopy(this, (LPVARIANT)pSrc));
}

COleVariant::COleVariant(const COleVariant& varSrc)
{
        AfxVariantInit(this);
        CheckError(::VariantCopy(this, (LPVARIANT)&varSrc));
}

COleVariant::COleVariant(LPCTSTR lpszSrc, VARTYPE vtSrc)
{
        USES_CONVERSION;
        ASSERT(vtSrc == VT_BSTR || vtSrc == VT_BSTRT);
        UNUSED(vtSrc);

        vt = VT_BSTR;
        bstrVal = NULL;

        if (lpszSrc != NULL)
        {
#ifndef _UNICODE
                if (vtSrc == VT_BSTRT)
                {
                        int nLen = lstrlen(lpszSrc);
                        bstrVal = ::SysAllocStringByteLen(lpszSrc, nLen);
                }
                else
#endif
                {
                        bstrVal = ::SysAllocString(T2COLE(lpszSrc));
                }

                /*
                BUGBUG:
                if (bstrVal == NULL)
                        AfxThrowMemoryException();
                */
        }
}

void COleVariant::SetString(LPCTSTR lpszSrc, VARTYPE vtSrc)
{
        USES_CONVERSION;
        ASSERT(vtSrc == VT_BSTR || vtSrc == VT_BSTRT);
        UNUSED(vtSrc);

        // Free up previous VARIANT
        Clear();

        vt = VT_BSTR;
        bstrVal = NULL;

        if (lpszSrc != NULL)
        {
#ifndef _UNICODE
                if (vtSrc == VT_BSTRT)
                {
                        int nLen = lstrlen(lpszSrc);
                        bstrVal = ::SysAllocStringByteLen(lpszSrc, nLen);
                }
                else
#endif
                {
                        bstrVal = ::SysAllocString(T2COLE(lpszSrc));
                }

                /*
                BUGBUG:
                if (bstrVal == NULL)
                        AfxThrowMemoryException();
                */
        }
}

COleVariant::COleVariant(short nSrc, VARTYPE vtSrc)
{
        ASSERT(vtSrc == VT_I2 || vtSrc == VT_BOOL);

        if (vtSrc == VT_BOOL)
        {
                vt = VT_BOOL;
                if (nSrc == FALSE)
                        V_BOOL(this) = AFX_OLE_FALSE;
                else
                        V_BOOL(this) = AFX_OLE_TRUE;
        }
        else
        {
                vt = VT_I2;
                iVal = nSrc;
        }
}

COleVariant::COleVariant(long lSrc, VARTYPE vtSrc)
{
        ASSERT(vtSrc == VT_I4 || vtSrc == VT_ERROR || vtSrc == VT_BOOL);

        if (vtSrc == VT_ERROR)
        {
                vt = VT_ERROR;
                scode = lSrc;
        }
        else if (vtSrc == VT_BOOL)
        {
                vt = VT_BOOL;
                if (lSrc == FALSE)
                        V_BOOL(this) = AFX_OLE_FALSE;
                else
                        V_BOOL(this) = AFX_OLE_TRUE;
        }
        else
        {
                vt = VT_I4;
                lVal = lSrc;
        }
}

// Operations
void COleVariant::Clear()
{
        VERIFY(::VariantClear(this) == NOERROR);
}

void COleVariant::ChangeType(VARTYPE vartype, LPVARIANT pSrc)
{
        // If pSrc is NULL, convert type in place
        if (pSrc == NULL)
                pSrc = this;
        if (pSrc != this || vartype != vt)
                CheckError(::VariantChangeType(this, pSrc, 0, vartype));
}

void COleVariant::Attach(VARIANT& varSrc)
{
        // Free up previous VARIANT
        Clear();

        // give control of data to COleVariant
        memcpy(this, &varSrc, sizeof(varSrc));
        varSrc.vt = VT_EMPTY;
}

VARIANT COleVariant::Detach()
{
        VARIANT varResult = *this;
        vt = VT_EMPTY;
        return varResult;
}

// Literal comparison. Types and values must match.
BOOL COleVariant::operator==(const VARIANT& var) const
{
        if (&var == this)
                return TRUE;

        // Variants not equal if types don't match
        if (var.vt != vt)
                return FALSE;

        // Check type specific values
        switch (vt)
        {
        case VT_EMPTY:
        case VT_NULL:
                return TRUE;

        case VT_BOOL:
                return V_BOOL(&var) == V_BOOL(this);

        case VT_UI1:
                return var.bVal == bVal;

        case VT_I2:
                return var.iVal == iVal;

        case VT_I4:
                return var.lVal == lVal;

        case VT_CY:
                return (var.cyVal.Hi == cyVal.Hi && var.cyVal.Lo == cyVal.Lo);

        case VT_R4:
                return var.fltVal == fltVal;

        case VT_R8:
                return var.dblVal == dblVal;

        case VT_DATE:
                return var.date == date;

        case VT_BSTR:
                return SysStringByteLen(var.bstrVal) == SysStringByteLen(bstrVal) &&
                        memcmp(var.bstrVal, bstrVal, SysStringByteLen(bstrVal)) == 0;

        case VT_ERROR:
                return var.scode == scode;

        case VT_DISPATCH:
        case VT_UNKNOWN:
                return var.punkVal == punkVal;

        default:
                if (vt & VT_ARRAY && !(vt & VT_BYREF))
                        return CompareSafeArrays(var.parray, parray);
                else
                        ASSERT(FALSE);  // VT_BYREF not supported
                // fall through
        }

        return FALSE;
}

const COleVariant& COleVariant::operator=(const VARIANT& varSrc)
{
        CheckError(::VariantCopy(this, (LPVARIANT)&varSrc));
        return *this;
}

const COleVariant& COleVariant::operator=(LPCVARIANT pSrc)
{
        CheckError(::VariantCopy(this, (LPVARIANT)pSrc));
        return *this;
}

const COleVariant& COleVariant::operator=(const COleVariant& varSrc)
{
        CheckError(::VariantCopy(this, (LPVARIANT)&varSrc));
        return *this;
}

const COleVariant& COleVariant::operator=(const LPCTSTR lpszSrc)
{
        USES_CONVERSION;
        // Free up previous VARIANT
        Clear();

        vt = VT_BSTR;
        if (lpszSrc == NULL)
                bstrVal = NULL;
        else
        {
                bstrVal = ::SysAllocString(T2COLE(lpszSrc));
                /*
                BUGBUG:
                if (bstrVal == NULL)
                        AfxThrowMemoryException();
                */
        }
        return *this;
}

const COleVariant& COleVariant::operator=(const CString& strSrc)
{
        USES_CONVERSION;
        // Free up previous VARIANT
        Clear();

        vt = VT_BSTR;
        bstrVal = ::SysAllocString(T2COLE(strSrc));
        /*
        BUGBUG:
        if (bstrVal == NULL)
                AfxThrowMemoryException();
        */

        return *this;
}

const COleVariant& COleVariant::operator=(BYTE nSrc)
{
        // Free up previous VARIANT if necessary
        if (vt != VT_UI1)
        {
                Clear();
                vt = VT_UI1;
        }

        bVal = nSrc;
        return *this;
}

const COleVariant& COleVariant::operator=(short nSrc)
{
        if (vt == VT_I2)
                iVal = nSrc;
        else if (vt == VT_BOOL)
        {
                if (nSrc == FALSE)
                        V_BOOL(this) = AFX_OLE_FALSE;
                else
                        V_BOOL(this) = AFX_OLE_TRUE;
        }
        else
        {
                // Free up previous VARIANT
                Clear();
                vt = VT_I2;
                iVal = nSrc;
        }

        return *this;
}

const COleVariant& COleVariant::operator=(long lSrc)
{
        if (vt == VT_I4)
                lVal = lSrc;
        else if (vt == VT_ERROR)
                scode = lSrc;
        else if (vt == VT_BOOL)
        {
                if (lSrc == FALSE)
                        V_BOOL(this) = AFX_OLE_FALSE;
                else
                        V_BOOL(this) = AFX_OLE_TRUE;
        }
        else
        {
                // Free up previous VARIANT
                Clear();
                vt = VT_I4;
                lVal = lSrc;
        }

        return *this;
}

/*
const COleVariant& COleVariant::operator=(const COleCurrency& curSrc)
{
        // Free up previous VARIANT if necessary
        if (vt != VT_CY)
        {
                Clear();
                vt = VT_CY;
        }

        cyVal = curSrc.m_cur;
        return *this;
}
*/
const COleVariant& COleVariant::operator=(float fltSrc)
{
        // Free up previous VARIANT if necessary
        if (vt != VT_R4)
        {
                Clear();
                vt = VT_R4;
        }

        fltVal = fltSrc;
        return *this;
}

const COleVariant& COleVariant::operator=(double dblSrc)
{
        // Free up previous VARIANT if necessary
        if (vt != VT_R8)
        {
                Clear();
                vt = VT_R8;
        }

        dblVal = dblSrc;
        return *this;
}

const COleVariant& COleVariant::operator=(const COleDateTime& dateSrc)
{
        // Free up previous VARIANT if necessary
        if (vt != VT_DATE)
        {
                Clear();
                vt = VT_DATE;
        }

        date = dateSrc.m_dt;
        return *this;
}
/*
const COleVariant& COleVariant::operator=(const CByteArray& arrSrc)
{
        int nSize = arrSrc.GetSize();

        // Set the correct type and make sure SafeArray can hold data
        CreateOneDimArray(*this, (DWORD)nSize);

        // Copy the data into the SafeArray
        CopyBinaryData(parray, arrSrc.GetData(), (DWORD)nSize);

        return *this;
}
const COleVariant& COleVariant::operator=(const CLongBinary& lbSrc)
{
        // Set the correct type and make sure SafeArray can hold data
        CreateOneDimArray(*this, lbSrc.m_dwDataLength);

        // Copy the data into the SafeArray
        BYTE* pData = (BYTE*)::GlobalLock(lbSrc.m_hData);
        CopyBinaryData(parray, pData, lbSrc.m_dwDataLength);
        ::GlobalUnlock(lbSrc.m_hData);

        return *this;
}
*/

void AFXAPI AfxVariantInit(LPVARIANT pVar)
{
        memset(pVar, 0, sizeof(*pVar));
}

/////////////////////////////////////////////////////////////////////////////
// Diagnostics

#ifdef _DEBUG
CDumpContext& AFXAPI operator <<(CDumpContext& dc, COleVariant varSrc)
{
        LPCVARIANT pSrc = (LPCVARIANT)varSrc;

        dc << "\nCOleVariant Object:";
        dc << "\n\t vt = " << pSrc->vt;

        // No support for VT_BYREF & VT_ARRAY
        if (pSrc->vt & VT_BYREF || pSrc->vt & VT_ARRAY)
                return dc;

        switch (pSrc->vt)
        {
        case VT_BOOL:
                return dc << "\n\t VT_BOOL = " << V_BOOL(pSrc);

        case VT_UI1:
                return dc << "\n\t bVal = " << pSrc->bVal;

        case VT_I2:
                return dc << "\n\t iVal = " << pSrc->iVal;

        case VT_I4:
                return dc << "\n\t lVal = " << pSrc->lVal;

        case VT_CY:
                {
                        COleVariant var(varSrc);
                        var.ChangeType(VT_BSTR);
                        return dc << "\n\t cyVal = " << (LPCTSTR)var.bstrVal;
                }

        case VT_R4:
                return dc << "\n\t fltVal = " << pSrc->fltVal;

        case VT_R8:
                return dc << "\n\t dblVal = " << pSrc->dblVal;

        case VT_DATE:
                {
                        COleVariant var(varSrc);
                        var.ChangeType(VT_BSTR);
                        return dc << "\n\t date = " << (LPCTSTR)var.bstrVal;
                }

        case VT_BSTR:
                return dc << "\n\t bstrVal = " << (LPCTSTR)pSrc->bstrVal;

        case VT_ERROR:
                return dc << "\n\t scode = " << pSrc->scode;

        case VT_DISPATCH:
        case VT_UNKNOWN:
                return dc << "\n\t punkVal = " << pSrc->punkVal;

        case VT_EMPTY:
        case VT_NULL:
                return dc;

        default:
                ASSERT(FALSE);
                return dc;
        }
}
#endif // _DEBUG


#ifdef _with_archive_
CArchive& AFXAPI operator<<(CArchive& ar, COleVariant varSrc)
{
        LPCVARIANT pSrc = (LPCVARIANT)varSrc;

        ar << pSrc->vt;

        // No support for VT_BYREF & VT_ARRAY
        if (pSrc->vt & VT_BYREF || pSrc->vt & VT_ARRAY)
                return ar;

        switch (pSrc->vt)
        {
        case VT_BOOL:
                return ar << (WORD)V_BOOL(pSrc);

        case VT_UI1:
                return ar << pSrc->bVal;

        case VT_I2:
                return ar << (WORD)pSrc->iVal;

        case VT_I4:
                return ar << pSrc->lVal;

        case VT_CY:
                ar << pSrc->cyVal.Lo;
                return ar << pSrc->cyVal.Hi;

        case VT_R4:
                return ar << pSrc->fltVal;

        case VT_R8:
                return ar << pSrc->dblVal;

        case VT_DATE:
                return ar << pSrc->date;

        case VT_BSTR:
                {
                        DWORD nLen = SysStringByteLen(pSrc->bstrVal);
                        ar << nLen;
                        if (nLen > 0)
                                ar.Write(pSrc->bstrVal, nLen * sizeof(BYTE));

                        return ar;
                }

        case VT_ERROR:
                return ar << pSrc->scode;

#ifdef _WITH_PUNK_
        case VT_DISPATCH:
        case VT_UNKNOWN:
                {
                        LPPERSISTSTREAM pPersistStream;
                        //BUGBUG
                        //CArchiveStream stm(&ar);

                        // QI for IPersistStream or IPeristStreamInit
                        SCODE sc = pSrc->punkVal->QueryInterface(
                                IID_IPersistStream, (void**)&pPersistStream);
//#ifndef _AFX_NO_OCC_SUPPORT
#ifdef _AFX_OCC_SUPPORT
                        if (FAILED(sc))
                                sc = pSrc->punkVal->QueryInterface(
                                        IID_IPersistStreamInit, (void**)&pPersistStream);
#endif
                        CheckError(sc);

                        TRY
                        {
                                // Get and archive the CLSID (GUID)
                                CLSID clsid;
                                CheckError(pPersistStream->GetClassID(&clsid));
                                ar << clsid.Data1;
                                ar << clsid.Data2;
                                ar << clsid.Data3;
                                ar.Write(&clsid.Data4[0], sizeof clsid.Data4);

                                // Always assume object is dirty
                                CheckError(pPersistStream->Save(&stm, TRUE));
                        }
                        CATCH_ALL(e)
                        {
                                pPersistStream->Release();
                                THROW_LAST();
                        }
                        END_CATCH_ALL
                        pPersistStream->Release();
                }
                return ar;
#endif //_WITH_PUNK_

        case VT_EMPTY:
        case VT_NULL:
                // do nothing
                return ar;

        default:
                ASSERT(FALSE);
                return ar;
        }
}

CArchive& AFXAPI operator>>(CArchive& ar, COleVariant& varSrc)
{
        LPVARIANT pSrc = &varSrc;

        // Free up current data if necessary
        if (pSrc->vt != VT_EMPTY)
                VariantClear(pSrc);
        ar >> pSrc->vt;

        // No support for VT_BYREF & VT_ARRAY
        if (pSrc->vt & VT_BYREF || pSrc->vt & VT_ARRAY)
                return ar;

        switch (pSrc->vt)
        {
        case VT_BOOL:
                return ar >> (WORD&)V_BOOL(pSrc);

        case VT_UI1:
                return ar >> pSrc->bVal;

        case VT_I2:
                return ar >> (WORD&)pSrc->iVal;

        case VT_I4:
                return ar >> pSrc->lVal;

        case VT_CY:
                ar >> pSrc->cyVal.Lo;
                return ar >> pSrc->cyVal.Hi;

        case VT_R4:
                return ar >> pSrc->fltVal;

        case VT_R8:
                return ar >> pSrc->dblVal;

        case VT_DATE:
                return ar >> pSrc->date;

        case VT_BSTR:
                {
                        DWORD nLen;
                        ar >> nLen;
                        if (nLen > 0)
                        {
                                pSrc->bstrVal = SysAllocStringByteLen(NULL, nLen);
                                //BUGBUG:
                                //if (pSrc->bstrVal == NULL)
                                //        AfxThrowMemoryException();

                                ar.Read(pSrc->bstrVal, nLen * sizeof(BYTE));
                        }
                        else
                                pSrc->bstrVal = NULL;

                        return ar;
                }
                break;

        case VT_ERROR:
                return ar >> pSrc->scode;

#ifdef _WITH_PUNK_
        case VT_DISPATCH:
        case VT_UNKNOWN:
                {
                        LPPERSISTSTREAM pPersistStream = NULL;
                        CArchiveStream stm(&ar);

                        // Retrieve the CLSID (GUID) and create an instance
                        CLSID clsid;
                        ar >> clsid.Data1;
                        ar >> clsid.Data2;
                        ar >> clsid.Data3;
                        ar.Read(&clsid.Data4[0], sizeof clsid.Data4);

                        // Create the object
                        SCODE sc = CoCreateInstance(clsid, NULL, CLSCTX_ALL | CLSCTX_REMOTE_SERVER,
                                pSrc->vt == VT_UNKNOWN ? IID_IUnknown : IID_IDispatch,
                                (void**)&pSrc->punkVal);
                        if (sc == E_INVALIDARG)
                        {
                                // may not support CLSCTX_REMOTE_SERVER, so try without
                                sc = CoCreateInstance(clsid, NULL,
                                        CLSCTX_ALL & ~CLSCTX_REMOTE_SERVER,
                                        pSrc->vt == VT_UNKNOWN ? IID_IUnknown : IID_IDispatch,
                                        (void**)&pSrc->punkVal);
                        }
                        CheckError(sc);

                        TRY
                        {
                                // QI for IPersistStream or IPeristStreamInit
                                sc = pSrc->punkVal->QueryInterface(
                                        IID_IPersistStream, (void**)&pPersistStream);
#ifndef _AFX_NO_OCC_SUPPORT
                                if (FAILED(sc))
                                        sc = pSrc->punkVal->QueryInterface(
                                                IID_IPersistStreamInit, (void**)&pPersistStream);
#endif
                                CheckError(sc);

                                // Always assumes object is dirty
                                CheckError(pPersistStream->Load(&stm));
                        }
                        CATCH_ALL(e)
                        {
                                // Clean up
                                if (pPersistStream != NULL)
                                        pPersistStream->Release();

                                pSrc->punkVal->Release();
                                THROW_LAST();
                        }
                        END_CATCH_ALL

                        pPersistStream->Release();
                }
                return ar;
#endif //_WITH_PUNK_

        case VT_EMPTY:
        case VT_NULL:
                // do nothing
                return ar;

        default:
                ASSERT(FALSE);
                return ar;
        }
}
#endif //_with_archive_

/////////////////////////////////////////////////////////////////////////////
// COleVariant Helpers

void AFXAPI ConstructElements(COleVariant* pElements, int nCount)
{
        ASSERT(nCount == 0 ||
                AfxIsValidAddress(pElements, nCount * sizeof(COleVariant)));

        for (; nCount--; ++pElements)
                new(pElements) COleVariant;
}

void AFXAPI DestructElements(COleVariant* pElements, int nCount)
{
        ASSERT(nCount == 0 ||
                AfxIsValidAddress(pElements, nCount * sizeof(COleVariant)));

        for (; nCount--; ++pElements)
                pElements->~COleVariant();
}

void AFXAPI CopyElements(COleVariant* pDest, const COleVariant* pSrc, int nCount)
{
        ASSERT(nCount == 0 ||
                AfxIsValidAddress(pDest, nCount * sizeof(COleVariant)));
        ASSERT(nCount == 0 ||
                AfxIsValidAddress(pSrc, nCount * sizeof(COleVariant)));

        for (; nCount--; ++pDest, ++pSrc)
                *pDest = *pSrc;
}

#ifdef _with_archive_
void AFXAPI SerializeElements(CArchive& ar, COleVariant* pElements, int nCount)
{
        ASSERT(nCount == 0 ||
                AfxIsValidAddress(pElements, nCount * sizeof(COleVariant)));

        if (ar.IsStoring())
        {
                for (; nCount--; ++pElements)
                        ar << *pElements;
        }
        else
        {
                for (; nCount--; ++pElements)
                        ar >> *pElements;
        }
}
#endif //_with_archive_

#ifdef _DEBUG
void AFXAPI DumpElements(CDumpContext& dc, COleVariant* pElements, int nCount)
{
        for (; nCount--; ++pElements)
                dc << *pElements;
}
#endif // _DEBUG

UINT AFXAPI HashKey(const struct tagVARIANT& var)
{
        switch (var.vt)
        {
        case VT_EMPTY:
        case VT_NULL:
                return 0;
        case VT_I2:
                return HashKey((DWORD)var.iVal);
        case VT_I4:
                return HashKey((DWORD)var.lVal);
        case VT_R4:
                return (UINT)(var.fltVal / 16);
        case VT_R8:
        case VT_CY:
                return (UINT)(var.dblVal / 16);
        case VT_BOOL:
                return HashKey((DWORD)V_BOOL(&var));
        case VT_ERROR:
                return HashKey((DWORD)var.scode);
        case VT_DATE:
                return (UINT)(var.date / 16);
        case VT_BSTR:
                return HashKey(var.bstrVal);
        case VT_DISPATCH:
        case VT_UNKNOWN:
                return HashKey((DWORD_PTR)var.punkVal);

        default:
                // No support for VT_BYREF & VT_ARRAY
                ASSERT(FALSE);

                // Fall through
        }

        return 0;
}

static void PASCAL CheckError(SCODE sc)
{
        if (FAILED(sc))
        {
            /*
            BUGBUG:
                if (sc == E_OUTOFMEMORY)
                        AfxThrowMemoryException();
                else
                        AfxThrowOleException(sc);
            */
        }
}
static BOOL PASCAL CompareSafeArrays(SAFEARRAY* parray1, SAFEARRAY* parray2)
{
    return FALSE;
}

#ifdef _CURRENCY_ALSO_
static BOOL PASCAL CompareSafeArrays(SAFEARRAY* parray1, SAFEARRAY* parray2)
{
        BOOL bCompare = FALSE;

        // If one is NULL they must both be NULL to compare
        if (parray1 == NULL || parray2 == NULL)
        {
                return parray1 == parray2;
        }

        // Dimension must match and if 0, then arrays compare
        DWORD dwDim1 = ::SafeArrayGetDim(parray1);
        DWORD dwDim2 = ::SafeArrayGetDim(parray2);
        if (dwDim1 != dwDim2)
                return FALSE;
        else if (dwDim1 == 0)
                return TRUE;

        // Element size must match
        DWORD dwSize1 = ::SafeArrayGetElemsize(parray1);
        DWORD dwSize2 = ::SafeArrayGetElemsize(parray2);
        if (dwSize1 != dwSize2)
                return FALSE;

        long* pLBound1 = NULL;
        long* pLBound2 = NULL;
        long* pUBound1 = NULL;
        long* pUBound2 = NULL;

        void* pData1 = NULL;
        void* pData2 = NULL;

        TRY
        {
                // Bounds must match
                pLBound1 = new long[dwDim1];
                pLBound2 = new long[dwDim2];
                pUBound1 = new long[dwDim1];
                pUBound2 = new long[dwDim2];

                size_t nTotalElements = 1;

                // Get and compare bounds
                for (DWORD dwIndex = 0; dwIndex < dwDim1; dwIndex++)
                {
                        CheckError(::SafeArrayGetLBound(
                                parray1, dwIndex+1, &pLBound1[dwIndex]));
                        CheckError(::SafeArrayGetLBound(
                                parray2, dwIndex+1, &pLBound2[dwIndex]));
                        CheckError(::SafeArrayGetUBound(
                                parray1, dwIndex+1, &pUBound1[dwIndex]));
                        CheckError(::SafeArrayGetUBound(
                                parray2, dwIndex+1, &pUBound2[dwIndex]));

                        // Check the magnitude of each bound
                        if (pUBound1[dwIndex] - pLBound1[dwIndex] !=
                                pUBound2[dwIndex] - pLBound2[dwIndex])
                        {
                                delete[] pLBound1;
                                delete[] pLBound2;
                                delete[] pUBound1;
                                delete[] pUBound2;

                                return FALSE;
                        }

                        // Increment the element count
                        nTotalElements *= pUBound1[dwIndex] - pLBound1[dwIndex] + 1;
                }

                // Access the data
                CheckError(::SafeArrayAccessData(parray1, &pData1));
                CheckError(::SafeArrayAccessData(parray2, &pData2));

                // Calculate the number of bytes of data and compare
                size_t nSize = nTotalElements * dwSize1;
                int nOffset = memcmp(pData1, pData2, nSize);
                bCompare = nOffset == 0;

                // Release the array locks
                CheckError(::SafeArrayUnaccessData(parray1));
                CheckError(::SafeArrayUnaccessData(parray2));
        }
        CATCH_ALL(e)
        {
                // Clean up bounds arrays
                delete[] pLBound1;
                delete[] pLBound2;
                delete[] pUBound1;
                delete[] pUBound2;

                // Release the array locks
                if (pData1 != NULL)
                        CheckError(::SafeArrayUnaccessData(parray1));
                if (pData2 != NULL)
                        CheckError(::SafeArrayUnaccessData(parray2));

                THROW_LAST();
        }
        END_CATCH_ALL

        // Clean up bounds arrays
        delete[] pLBound1;
        delete[] pLBound2;
        delete[] pUBound1;
        delete[] pUBound2;

        return bCompare;
}

static void PASCAL CreateOneDimArray(VARIANT& varSrc, DWORD dwSize)
{
        UINT nDim;

        // Clear VARIANT and re-create SafeArray if necessary
        if (varSrc.vt != (VT_UI1 | VT_ARRAY) ||
                (nDim = ::SafeArrayGetDim(varSrc.parray)) != 1)
        {
                VERIFY(::VariantClear(&varSrc) == NOERROR);
                varSrc.vt = VT_UI1 | VT_ARRAY;

                SAFEARRAYBOUND bound;
                bound.cElements = dwSize;
                bound.lLbound = 0;
                varSrc.parray = ::SafeArrayCreate(VT_UI1, 1, &bound);
                if (varSrc.parray == NULL)
                        AfxThrowMemoryException();
        }
        else
        {
                // Must redimension array if necessary
                long lLower, lUpper;
                CheckError(::SafeArrayGetLBound(varSrc.parray, 1, &lLower));
                CheckError(::SafeArrayGetUBound(varSrc.parray, 1, &lUpper));

                // Upper bound should always be greater than lower bound
                long lSize = lUpper - lLower;
                if (lSize < 0)
                {
                        ASSERT(FALSE);
                        lSize = 0;

                }

                if ((DWORD)lSize != dwSize)
                {
                        SAFEARRAYBOUND bound;
                        bound.cElements = dwSize;
                        bound.lLbound = lLower;
                        CheckError(::SafeArrayRedim(varSrc.parray, &bound));
                }
        }
}

static void PASCAL CopyBinaryData(SAFEARRAY* parray, const void* pvSrc, DWORD dwSize)
{
        // Access the data, copy it and unaccess it.
        void* pDest;
        CheckError(::SafeArrayAccessData(parray, &pDest));
        memcpy(pDest, pvSrc, dwSize);
        CheckError(::SafeArrayUnaccessData(parray));
}

/////////////////////////////////////////////////////////////////////////////
// COleCurrency class helpers

// Return the highest order bit composing dwTarget in wBit
#define HI_BIT(dwTarget, wBit) \
        do \
        { \
                if (dwTarget != 0) \
                        for (wBit = 32; (dwTarget & (0x00000001 << wBit-1)) == 0; wBit--);\
                else \
                        wBit = 0; \
        } while (0)

// Left shift an (assumed unsigned) currency by wBits
#define LSHIFT_UCUR(cur, wBits) \
        do \
        { \
                for (WORD wTempBits = wBits; wTempBits > 0; wTempBits--) \
                { \
                        cur.m_cur.Hi = ((DWORD)cur.m_cur.Hi << 1); \
                        cur.m_cur.Hi |= (cur.m_cur.Lo & 0x80000000) >> 31; \
                        cur.m_cur.Lo = cur.m_cur.Lo << 1; \
                } \
        } while (0)

// Right shift an (assumed unsigned) currency by wBits
#define RSHIFT_UCUR(cur, wBits) \
        do \
        { \
                for (WORD wTempBits = wBits; wTempBits > 0; wTempBits--) \
                { \
                        cur.m_cur.Lo = cur.m_cur.Lo >> 1; \
                        cur.m_cur.Lo |= (cur.m_cur.Hi & 0x00000001) << 31; \
                        cur.m_cur.Hi = ((DWORD)cur.m_cur.Hi >> 1); \
                } \
        } while (0)

/////////////////////////////////////////////////////////////////////////////
// COleCurrency class (internally currency is 8-byte int scaled by 10,000)

COleCurrency::COleCurrency(long nUnits, long nFractionalUnits)
{
        SetCurrency(nUnits, nFractionalUnits);
        SetStatus(valid);
}

const COleCurrency& COleCurrency::operator=(CURRENCY cySrc)
{
        m_cur = cySrc;
        SetStatus(valid);
        return *this;
}

const COleCurrency& COleCurrency::operator=(const COleCurrency& curSrc)
{
        m_cur = curSrc.m_cur;
        m_status = curSrc.m_status;
        return *this;
}

const COleCurrency& COleCurrency::operator=(const VARIANT& varSrc)
{
        if (varSrc.vt != VT_CY)
        {
                TRY
                {
                        COleVariant varTemp(varSrc);
                        varTemp.ChangeType(VT_CY);
                        m_cur = varTemp.cyVal;
                        SetStatus(valid);
                }
                // Catch COleException from ChangeType, but not CMemoryException
                CATCH(COleException, e)
                {
                        // Not able to convert VARIANT to CURRENCY
                        m_cur.Hi = 0;
                        m_cur.Lo = 0;
                        SetStatus(invalid);
                        DELETE_EXCEPTION(e);
                }
                END_CATCH
        }
        else
        {
                m_cur = varSrc.cyVal;
                SetStatus(valid);
        }

        return *this;
}

BOOL COleCurrency::operator<(const COleCurrency& cur) const
{
        ASSERT(GetStatus() == valid);
        ASSERT(cur.GetStatus() == valid);

        return((m_cur.Hi == cur.m_cur.Hi) ?
                (m_cur.Lo < cur.m_cur.Lo) : (m_cur.Hi < cur.m_cur.Hi));
}

BOOL COleCurrency::operator>(const COleCurrency& cur) const
{
        ASSERT(GetStatus() == valid);
        ASSERT(cur.GetStatus() == valid);

        return((m_cur.Hi == cur.m_cur.Hi) ?
                (m_cur.Lo > cur.m_cur.Lo) : (m_cur.Hi > cur.m_cur.Hi));
}

BOOL COleCurrency::operator<=(const COleCurrency& cur) const
{
        ASSERT(GetStatus() == valid);
        ASSERT(cur.GetStatus() == valid);

        return((m_cur.Hi == cur.m_cur.Hi) ?
                (m_cur.Lo <= cur.m_cur.Lo) : (m_cur.Hi < cur.m_cur.Hi));
}

BOOL COleCurrency::operator>=(const COleCurrency& cur) const
{
        ASSERT(GetStatus() == valid);
        ASSERT(cur.GetStatus() == valid);

        return((m_cur.Hi == cur.m_cur.Hi) ?
                (m_cur.Lo >= cur.m_cur.Lo) : (m_cur.Hi > cur.m_cur.Hi));
}

COleCurrency COleCurrency::operator+(const COleCurrency& cur) const
{
        COleCurrency curResult;

        // If either operand Null, result Null
        if (GetStatus() == null || cur.GetStatus() == null)
        {
                curResult.SetStatus(null);
                return curResult;
        }

        // If either operand Invalid, result Invalid
        if (GetStatus() == invalid || cur.GetStatus() == invalid)
        {
                curResult.SetStatus(invalid);
                return curResult;
        }

        // Add separate CURRENCY components
        curResult.m_cur.Hi = m_cur.Hi + cur.m_cur.Hi;
        curResult.m_cur.Lo = m_cur.Lo + cur.m_cur.Lo;

        // Increment Hi if Lo overflows
        if (m_cur.Lo > curResult.m_cur.Lo)
                curResult.m_cur.Hi++;

        // Overflow if operands same sign and result sign different
        if (!((m_cur.Hi ^ cur.m_cur.Hi) & 0x80000000) &&
                ((m_cur.Hi ^ curResult.m_cur.Hi) & 0x80000000))
        {
                curResult.SetStatus(invalid);
        }

        return curResult;
}

COleCurrency COleCurrency::operator-(const COleCurrency& cur) const
{
        COleCurrency curResult;

        // If either operand Null, result Null
        if (GetStatus() == null || cur.GetStatus() == null)
        {
                curResult.SetStatus(null);
                return curResult;
        }

        // If either operand Invalid, result Invalid
        if (GetStatus() == invalid || cur.GetStatus() == invalid)
        {
                curResult.SetStatus(invalid);
                return curResult;
        }

        // Subtract separate CURRENCY components
        curResult.m_cur.Hi = m_cur.Hi - cur.m_cur.Hi;
        curResult.m_cur.Lo = m_cur.Lo - cur.m_cur.Lo;

        // Decrement Hi if Lo overflows
        if (m_cur.Lo < curResult.m_cur.Lo)
                curResult.m_cur.Hi--;

        // Overflow if operands not same sign and result not same sign
        if (((m_cur.Hi ^ cur.m_cur.Hi) & 0x80000000) &&
                ((m_cur.Hi ^ curResult.m_cur.Hi) & 0x80000000))
        {
                curResult.SetStatus(invalid);
        }

        return curResult;
}

COleCurrency COleCurrency::operator-() const
{
        // If operand not Valid, just return
        if (!GetStatus() == valid)
                return *this;

        COleCurrency curResult;

        // Negating MIN_CURRENCY,will set invalid
        if (m_cur.Hi == 0x80000000 && m_cur.Lo == 0x00000000)
        {
                curResult.SetStatus(invalid);
        }

        curResult.m_cur.Hi = ~m_cur.Hi;
        curResult.m_cur.Lo = -(long)m_cur.Lo;

        // If cy was -1 make sure Hi correctly set
        if (curResult.m_cur.Lo == 0)
                curResult.m_cur.Hi++;

        return curResult;
}

COleCurrency COleCurrency::operator*(long nOperand) const
{
        // If operand not Valid, just return
        if (!GetStatus() == valid)
                return *this;

        COleCurrency curResult(m_cur);
        DWORD nTempOp;

        // Return now if one operand is 0 (optimization)
        if ((m_cur.Hi == 0x00000000 && m_cur.Lo == 0x00000000) || nOperand == 0)
        {
                curResult.m_cur.Hi = 0;
                curResult.m_cur.Lo = 0;
                return curResult;
        }

        // Handle only valid case of multiplying MIN_CURRENCY
        if (m_cur.Hi == 0x80000000 && m_cur.Lo == 0x00000000 && nOperand == 1)
                return curResult;

        // Compute absolute values.
        if (m_cur.Hi < 0)
                curResult = -curResult;

        nTempOp = labs(nOperand);

        // Check for overflow
        if (curResult.m_cur.Hi != 0)
        {
                WORD wHiBitCur, wHiBitOp;
                HI_BIT(curResult.m_cur.Hi, wHiBitCur);
                HI_BIT(nTempOp, wHiBitOp);

                // 63-bit limit on result. (n bits)*(m bits) = (n+m-1) bits.
                if (wHiBitCur + wHiBitOp - 1 > 63)
                {
                        // Overflow!
                        curResult.SetStatus(invalid);

                        // Set to maximum negative value
                        curResult.m_cur.Hi = 0x80000000;
                        curResult.m_cur.Lo = 0x00000000;

                        return curResult;
                }
        }

        // Break up into WORDs
        WORD wCy4, wCy3, wCy2, wCy1, wL2, wL1;

        wCy4 = HIWORD(curResult.m_cur.Hi);
        wCy3 = LOWORD(curResult.m_cur.Hi);
        wCy2 = HIWORD(curResult.m_cur.Lo);
        wCy1 = LOWORD(curResult.m_cur.Lo);

        wL2 = HIWORD(nTempOp);
        wL1 = LOWORD(nTempOp);

        // Multiply each set of WORDs
        DWORD dwRes11, dwRes12, dwRes21, dwRes22;
        DWORD dwRes31, dwRes32, dwRes41;  // Don't need dwRes42

        dwRes11 = wCy1 * wL1;
        dwRes12 = wCy1 * wL2;
        dwRes21 = wCy2 * wL1;
        dwRes22 = wCy2 * wL2;

        dwRes31 = wCy3 * wL1;
        dwRes32 = wCy3 * wL2;
        dwRes41 = wCy4 * wL1;

        // Add up low order pieces
        dwRes11 += dwRes12<<16;
        curResult.m_cur.Lo = dwRes11 + (dwRes21<<16);

        // Check if carry required
        if (dwRes11 < dwRes12<<16 || (DWORD)curResult.m_cur.Lo < dwRes11)
                curResult.m_cur.Hi = 1;
        else
                curResult.m_cur.Hi = 0;

        // Add up the high order pieces
        curResult.m_cur.Hi += dwRes31 + (dwRes32<<16) + (dwRes41<<16) +
                dwRes22 + (dwRes12>>16) + (dwRes21>>16);

        // Compute result sign
        if ((m_cur.Hi ^ nOperand) & 0x80000000)
                curResult = -curResult;

        return curResult;
}

COleCurrency COleCurrency::operator/(long nOperand) const
{
        // If operand not Valid, just return
        if (!GetStatus() == valid)
                return *this;

        COleCurrency curTemp(m_cur);
        DWORD nTempOp;

        // Check for divide by 0
        if (nOperand == 0)
        {
                curTemp.SetStatus(invalid);

                // Set to maximum negative value
                curTemp.m_cur.Hi = 0x80000000;
                curTemp.m_cur.Lo = 0x00000000;

                return curTemp;
        }

        // Compute absolute values
        if (curTemp.m_cur.Hi < 0)
                curTemp = -curTemp;

        nTempOp = labs(nOperand);

        // Optimization - division is simple if Hi == 0
        if (curTemp.m_cur.Hi == 0x0000)
        {
                curTemp.m_cur.Lo = m_cur.Lo / nTempOp;

                // Compute result sign
                if ((m_cur.Hi ^ nOperand) & 0x80000000)
                        curTemp = -curTemp;

                return curTemp;
        }

        // Now curTemp represents remainder
        COleCurrency curResult; // Initializes to zero
        COleCurrency curTempResult;
        COleCurrency curOperand;

        curOperand.m_cur.Lo = nTempOp;

        WORD wHiBitRem;
        WORD wScaleOp;

        // Quit if remainder can be truncated
        while (curTemp >= curOperand)
        {
                // Scale up and divide Hi portion
                HI_BIT(curTemp.m_cur.Hi, wHiBitRem);

                if (wHiBitRem != 0)
                        wHiBitRem += 32;
                else
                        HI_BIT(curTemp.m_cur.Lo, wHiBitRem);

                WORD wShift = (WORD)(64 - wHiBitRem);
                LSHIFT_UCUR(curTemp, wShift);

                // If Operand bigger than Hi it must be scaled
                wScaleOp = (WORD)((nTempOp > (DWORD)curTemp.m_cur.Hi) ? 1 : 0);

                // Perform synthetic division
                curTempResult.m_cur.Hi =
                        (DWORD)curTemp.m_cur.Hi / (nTempOp >> wScaleOp);

                // Scale back to get correct result and remainder
                RSHIFT_UCUR(curTemp, wShift);
                wShift = (WORD)(wShift - wScaleOp);
                RSHIFT_UCUR(curTempResult, wShift);

                // Now calculate result and remainder
                curResult += curTempResult;
                curTemp -= curTempResult * nTempOp;
        }

        // Compute result sign
        if ((m_cur.Hi ^ nOperand) & 0x80000000)
                curResult = -curResult;

        return curResult;
}

void COleCurrency::SetCurrency(long nUnits, long nFractionalUnits)
{
        COleCurrency curUnits;              // Initializes to 0
        COleCurrency curFractionalUnits;    // Initializes to 0

        // Set temp currency value to Units (need to multiply by 10,000)
        curUnits.m_cur.Lo = (DWORD)labs(nUnits);
        curUnits = curUnits * 10000;
        if (nUnits < 0)
                curUnits = -curUnits;

        curFractionalUnits.m_cur.Lo = (DWORD)labs(nFractionalUnits);
        if (nFractionalUnits < 0)
                curFractionalUnits = -curFractionalUnits;

        // Now add together Units and FractionalUnits
        *this = curUnits + curFractionalUnits;

        SetStatus(valid);
}

BOOL COleCurrency::ParseCurrency(LPCTSTR lpszCurrency,
        DWORD dwFlags,  LCID lcid)
{
        USES_CONVERSION;
        CString strCurrency = lpszCurrency;

        SCODE sc;
        if ( FAILED(sc = VarCyFromStr((LPOLESTR)T2COLE(strCurrency),
                lcid, dwFlags, &m_cur)))
        {
                if (sc == DISP_E_TYPEMISMATCH)
                {
                        // Can't convert string to CURRENCY, set 0 & invalid
                        m_cur.Hi = 0x00000000;
                        m_cur.Lo = 0x00000000;
                        SetStatus(invalid);
                        return FALSE;
                }
                else if (sc == DISP_E_OVERFLOW)
                {
                        // Can't convert string to CURRENCY, set max neg & invalid
                        m_cur.Hi = 0x80000000;
                        m_cur.Lo = 0x00000000;
                        SetStatus(invalid);
                        return FALSE;
                }
                else
                {
                        TRACE0("\nCOleCurrency VarCyFromStr call failed.\n\t");
                        if (sc == E_OUTOFMEMORY)
                                AfxThrowMemoryException();
                        else
                                AfxThrowOleException(sc);
                }
        }

        SetStatus(valid);
        return TRUE;
}

CString COleCurrency::Format(DWORD dwFlags, LCID lcid) const
{
        USES_CONVERSION;
        CString strCur;

        // If null, return empty string
        if (GetStatus() == null)
                return strCur;

        // If invalid, return Currency resource string
        if (GetStatus() == invalid)
        {
                VERIFY(strCur.LoadString(AFX_IDS_INVALID_CURRENCY));
                return strCur;
        }

        COleVariant var;
        // Don't need to trap error. Should not fail due to type mismatch
        CheckError(VarBstrFromCy(m_cur, lcid, dwFlags, &V_BSTR(&var)));
        var.vt = VT_BSTR;
        return OLE2CT(V_BSTR(&var));
}


// serialization
#ifdef _DEBUG
CDumpContext& AFXAPI operator<<(CDumpContext& dc, COleCurrency curSrc)
{
        dc << "\nCOleCurrency Object:";
        dc << "\n\tm_status = " << (long)curSrc.m_status;

        COleVariant var(curSrc);
        var.ChangeType(VT_CY);
        return dc << "\n\tCurrency = " << (LPCTSTR)var.bstrVal;
}
#endif // _DEBUG

#ifdef _with_archive_
CArchive& AFXAPI operator<<(CArchive& ar, COleCurrency curSrc)
{
        ar << (long)curSrc.m_status;
        ar << curSrc.m_cur.Hi;
        return ar << curSrc.m_cur.Lo;
}

CArchive& AFXAPI operator>>(CArchive& ar, COleCurrency& curSrc)
{
        ar >> (long&)curSrc.m_status;
        ar >> curSrc.m_cur.Hi;
        return ar >> curSrc.m_cur.Lo;
}
#endif //_with_archive_


/////////////////////////////////////////////////////////////////////////////
// COleDateTime class HELPER definitions

// Verifies will fail if the needed buffer size is too large
#define MAX_TIME_BUFFER_SIZE    128         // matches that in timecore.cpp
#define MIN_DATE                (-657434L)  // about year 100
#define MAX_DATE                2958465L    // about year 9999

// Half a second, expressed in days
#define HALF_SECOND  (1.0/172800.0)

// One-based array of days in year at month start
static int rgMonthDays[13] =
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

static BOOL OleDateFromTm(WORD wYear, WORD wMonth, WORD wDay,
        WORD wHour, WORD wMinute, WORD wSecond, DATE& dtDest);
static BOOL TmFromOleDate(DATE dtSrc, struct tm& tmDest);
static void TmConvertToStandardFormat(struct tm& tmSrc);
static double DoubleFromDate(DATE dt);
static DATE DateFromDouble(double dbl);

/////////////////////////////////////////////////////////////////////////////
// COleDateTime class

COleDateTime PASCAL COleDateTime::GetCurrentTime()
{
        return COleDateTime(::time(NULL));
}

int COleDateTime::GetYear() const
{
        struct tm tmTemp;

        if (GetStatus() == valid && TmFromOleDate(m_dt, tmTemp))
                return tmTemp.tm_year;
        else
                return AFX_OLE_DATETIME_ERROR;
}

int COleDateTime::GetMonth() const
{
        struct tm tmTemp;

        if (GetStatus() == valid && TmFromOleDate(m_dt, tmTemp))
                return tmTemp.tm_mon;
        else
                return AFX_OLE_DATETIME_ERROR;
}

int COleDateTime::GetDay() const
{
        struct tm tmTemp;

        if (GetStatus() == valid && TmFromOleDate(m_dt, tmTemp))
                return tmTemp.tm_mday;
        else
                return AFX_OLE_DATETIME_ERROR;
}

int COleDateTime::GetHour() const
{
        struct tm tmTemp;

        if (GetStatus() == valid && TmFromOleDate(m_dt, tmTemp))
                return tmTemp.tm_hour;
        else
                return AFX_OLE_DATETIME_ERROR;
}

int COleDateTime::GetMinute() const
{
        struct tm tmTemp;

        if (GetStatus() == valid && TmFromOleDate(m_dt, tmTemp))
                return tmTemp.tm_min;
        else
                return AFX_OLE_DATETIME_ERROR;
}

int COleDateTime::GetSecond() const
{
        struct tm tmTemp;

        if (GetStatus() == valid && TmFromOleDate(m_dt, tmTemp))
                return tmTemp.tm_sec;
        else
                return AFX_OLE_DATETIME_ERROR;
}

int COleDateTime::GetDayOfWeek() const
{
        struct tm tmTemp;

        if (GetStatus() == valid && TmFromOleDate(m_dt, tmTemp))
                return tmTemp.tm_wday;
        else
                return AFX_OLE_DATETIME_ERROR;
}

int COleDateTime::GetDayOfYear() const
{
        struct tm tmTemp;

        if (GetStatus() == valid && TmFromOleDate(m_dt, tmTemp))
                return tmTemp.tm_yday;
        else
                return AFX_OLE_DATETIME_ERROR;
}

const COleDateTime& COleDateTime::operator=(const VARIANT& varSrc)
{
        if (varSrc.vt != VT_DATE)
        {
                TRY
                {
                        COleVariant varTemp(varSrc);
                        varTemp.ChangeType(VT_DATE);
                        m_dt = varTemp.date;
                        SetStatus(valid);
                }
                // Catch COleException from ChangeType, but not CMemoryException
                CATCH(COleException, e)
                {
                        // Not able to convert VARIANT to DATE
                        DELETE_EXCEPTION(e);
                        m_dt = 0;
                        SetStatus(invalid);
                }
                END_CATCH
        }
        else
        {
                m_dt = varSrc.date;
                SetStatus(valid);
        }

        return *this;
}

const COleDateTime& COleDateTime::operator=(DATE dtSrc)
{
        m_dt = dtSrc;
        SetStatus(valid);

        return *this;
}

const COleDateTime& COleDateTime::operator=(const time_t& timeSrc)
{
        // Convert time_t to struct tm
        tm *ptm = localtime(&timeSrc);

        if (ptm != NULL)
        {
                m_status = OleDateFromTm((WORD)ptm->tm_year + 1900,
                        (WORD)(ptm->tm_mon + 1), (WORD)ptm->tm_mday,
                        (WORD)ptm->tm_hour, (WORD)ptm->tm_min,
                        (WORD)ptm->tm_sec, m_dt) ? valid : invalid;
        }
        else
        {
                // Local time must have failed (timsSrc before 1/1/70 12am)
                SetStatus(invalid);
                ASSERT(FALSE);
        }

        return *this;
}

const COleDateTime& COleDateTime::operator=(const SYSTEMTIME& systimeSrc)
{
        m_status = OleDateFromTm(systimeSrc.wYear, systimeSrc.wMonth,
                systimeSrc.wDay, systimeSrc.wHour, systimeSrc.wMinute,
                systimeSrc.wSecond, m_dt) ? valid : invalid;

        return *this;
}

const COleDateTime& COleDateTime::operator=(const FILETIME& filetimeSrc)
{
        // Assume UTC FILETIME, so convert to LOCALTIME
        FILETIME filetimeLocal;
        if (!FileTimeToLocalFileTime( &filetimeSrc, &filetimeLocal))
        {
#ifdef _DEBUG
                DWORD dwError = GetLastError();
                TRACE1("\nFileTimeToLocalFileTime failed. Error = %lu.\n\t", dwError);
#endif // _DEBUG
                m_status = invalid;
        }
        else
        {
                // Take advantage of SYSTEMTIME -> FILETIME conversion
                SYSTEMTIME systime;
                m_status = FileTimeToSystemTime(&filetimeLocal, &systime) ?
                        valid : invalid;

                // At this point systime should always be valid, but...
                if (GetStatus() == valid)
                {
                        m_status = OleDateFromTm(systime.wYear, systime.wMonth,
                                systime.wDay, systime.wHour, systime.wMinute,
                                systime.wSecond, m_dt) ? valid : invalid;
                }
        }

        return *this;
}

BOOL COleDateTime::operator<(const COleDateTime& date) const
{
        ASSERT(GetStatus() == valid);
        ASSERT(date.GetStatus() == valid);

        // Handle negative dates
        return DoubleFromDate(m_dt) < DoubleFromDate(date.m_dt);
}

BOOL COleDateTime::operator>(const COleDateTime& date) const
{   ASSERT(GetStatus() == valid);
        ASSERT(date.GetStatus() == valid);

        // Handle negative dates
        return DoubleFromDate(m_dt) > DoubleFromDate(date.m_dt);
}

BOOL COleDateTime::operator<=(const COleDateTime& date) const
{
        ASSERT(GetStatus() == valid);
        ASSERT(date.GetStatus() == valid);

        // Handle negative dates
        return DoubleFromDate(m_dt) <= DoubleFromDate(date.m_dt);
}

BOOL COleDateTime::operator>=(const COleDateTime& date) const
{
        ASSERT(GetStatus() == valid);
        ASSERT(date.GetStatus() == valid);

        // Handle negative dates
        return DoubleFromDate(m_dt) >= DoubleFromDate(date.m_dt);
}

COleDateTime COleDateTime::operator+(const COleDateTimeSpan& dateSpan) const
{
        COleDateTime dateResult;    // Initializes m_status to valid

        // If either operand NULL, result NULL
        if (GetStatus() == null || dateSpan.GetStatus() == null)
        {
                dateResult.SetStatus(null);
                return dateResult;
        }

        // If either operand invalid, result invalid
        if (GetStatus() == invalid || dateSpan.GetStatus() == invalid)
        {
                dateResult.SetStatus(invalid);
                return dateResult;
        }

        // Compute the actual date difference by adding underlying dates
        dateResult = DateFromDouble(DoubleFromDate(m_dt) + dateSpan.m_span);

        // Validate within range
        dateResult.CheckRange();

        return dateResult;
}

COleDateTime COleDateTime::operator-(const COleDateTimeSpan& dateSpan) const
{
        COleDateTime dateResult;    // Initializes m_status to valid

        // If either operand NULL, result NULL
        if (GetStatus() == null || dateSpan.GetStatus() == null)
        {
                dateResult.SetStatus(null);
                return dateResult;
        }

        // If either operand invalid, result invalid
        if (GetStatus() == invalid || dateSpan.GetStatus() == invalid)
        {
                dateResult.SetStatus(invalid);
                return dateResult;
        }

        // Compute the actual date difference by subtracting underlying dates
        dateResult = DateFromDouble(DoubleFromDate(m_dt) - dateSpan.m_span);

        // Validate within range
        dateResult.CheckRange();

        return dateResult;
}

COleDateTimeSpan COleDateTime::operator-(const COleDateTime& date) const
{
        COleDateTimeSpan spanResult;

        // If either operand NULL, result NULL
        if (GetStatus() == null || date.GetStatus() == null)
        {
                spanResult.SetStatus(COleDateTimeSpan::null);
                return spanResult;
        }

        // If either operand invalid, result invalid
        if (GetStatus() == invalid || date.GetStatus() == invalid)
        {
                spanResult.SetStatus(COleDateTimeSpan::invalid);
                return spanResult;
        }

        // Return result (span can't be invalid, so don't check range)
        return DoubleFromDate(m_dt) - DoubleFromDate(date.m_dt);
}

BOOL COleDateTime::SetDateTime(int nYear, int nMonth, int nDay,
        int nHour, int nMin, int nSec)
{
        return m_status = OleDateFromTm((WORD)nYear, (WORD)nMonth,
                (WORD)nDay, (WORD)nHour, (WORD)nMin, (WORD)nSec, m_dt) ?
                valid : invalid;
}

BOOL COleDateTime::ParseDateTime(LPCTSTR lpszDate, DWORD dwFlags, LCID lcid)
{
        USES_CONVERSION;
        CString strDate = lpszDate;

        SCODE sc;
        if (FAILED(sc = VarDateFromStr((LPOLESTR)T2COLE(strDate), lcid,
                dwFlags, &m_dt)))
        {
                if (sc == DISP_E_TYPEMISMATCH)
                {
                        // Can't convert string to date, set 0 and invalidate
                        m_dt = 0;
                        SetStatus(invalid);
                        return FALSE;
                }
                else if (sc == DISP_E_OVERFLOW)
                {
                        // Can't convert string to date, set -1 and invalidate
                        m_dt = -1;
                        SetStatus(invalid);
                        return FALSE;
                }
                else
                {
                        TRACE0("\nCOleDateTime VarDateFromStr call failed.\n\t");
                        if (sc == E_OUTOFMEMORY)
                                AfxThrowMemoryException();
                        else
                                AfxThrowOleException(sc);
                }
        }

        SetStatus(valid);
        return TRUE;
}

CString COleDateTime::Format(DWORD dwFlags, LCID lcid) const
{
        USES_CONVERSION;
        CString strDate;

        // If null, return empty string
        if (GetStatus() == null)
                return strDate;

        // If invalid, return DateTime resource string
        if (GetStatus() == invalid)
        {
                VERIFY(strDate.LoadString(AFX_IDS_INVALID_DATETIME));
                return strDate;
        }

        COleVariant var;
        // Don't need to trap error. Should not fail due to type mismatch
        CheckError(VarBstrFromDate(m_dt, lcid, dwFlags, &V_BSTR(&var)));
        var.vt = VT_BSTR;
        return OLE2CT(V_BSTR(&var));
}

CString COleDateTime::Format(LPCTSTR pFormat) const
{
        CString strDate;
        struct tm tmTemp;

        // If null, return empty string
        if (GetStatus() == null)
                return strDate;

        // If invalid, return DateTime resource string
        if (GetStatus() == invalid || !TmFromOleDate(m_dt, tmTemp))
        {
                VERIFY(strDate.LoadString(AFX_IDS_INVALID_DATETIME));
                return strDate;
        }

        // Convert tm from afx internal format to standard format
        TmConvertToStandardFormat(tmTemp);

        // Fill in the buffer, disregard return value as it's not necessary
        LPTSTR lpszTemp = strDate.GetBufferSetLength(MAX_TIME_BUFFER_SIZE);
        _tcsftime(lpszTemp, strDate.GetLength(), pFormat, &tmTemp);
        strDate.ReleaseBuffer();

        return strDate;
}

CString COleDateTime::Format(UINT nFormatID) const
{
        CString strFormat;
        VERIFY(strFormat.LoadString(nFormatID) != 0);
        return Format(strFormat);
}

void COleDateTime::CheckRange()
{
        if (m_dt > MAX_DATE || m_dt < MIN_DATE) // about year 100 to about 9999
                SetStatus(invalid);
}

// serialization
#ifdef _DEBUG
CDumpContext& AFXAPI operator<<(CDumpContext& dc, COleDateTime dateSrc)
{
        dc << "\nCOleDateTime Object:";
        dc << "\n\tm_status = " << (long)dateSrc.m_status;

        COleVariant var(dateSrc);
        var.ChangeType(VT_BSTR);

        return dc << "\n\tdate = " << (LPCTSTR)var.bstrVal;
}
#endif // _DEBUG

#ifdef _with_archive_
CArchive& AFXAPI operator<<(CArchive& ar, COleDateTime dateSrc)
{
        ar << (long)dateSrc.m_status;
        return ar << dateSrc.m_dt;
}

CArchive& AFXAPI operator>>(CArchive& ar, COleDateTime& dateSrc)
{
        ar >> (long&)dateSrc.m_status;
        return ar >> dateSrc.m_dt;
}
#endif //_with_archive_

/////////////////////////////////////////////////////////////////////////////
// COleDateTimeSpan class helpers

#define MAX_DAYS_IN_SPAN    3615897L

/////////////////////////////////////////////////////////////////////////////
// COleDateTimeSpan class
long COleDateTimeSpan::GetHours() const
{
        ASSERT(GetStatus() == valid);

        double dblTemp;

        // Truncate days and scale up
        dblTemp = modf(m_span, &dblTemp);
        return (long)(dblTemp * 24);
}

long COleDateTimeSpan::GetMinutes() const
{
        ASSERT(GetStatus() == valid);

        double dblTemp;

        // Truncate hours and scale up
        dblTemp = modf(m_span * 24, &dblTemp);
        return (long)(dblTemp * 60);
}

long COleDateTimeSpan::GetSeconds() const
{
        ASSERT(GetStatus() == valid);

        double dblTemp;

        // Truncate minutes and scale up
        dblTemp = modf(m_span * 24 * 60, &dblTemp);
        return (long)(dblTemp * 60);
}

const COleDateTimeSpan& COleDateTimeSpan::operator=(double dblSpanSrc)
{
        m_span = dblSpanSrc;
        SetStatus(valid);
        return *this;
}

const COleDateTimeSpan& COleDateTimeSpan::operator=(const COleDateTimeSpan& dateSpanSrc)
{
        m_span = dateSpanSrc.m_span;
        m_status = dateSpanSrc.m_status;
        return *this;
}

COleDateTimeSpan COleDateTimeSpan::operator+(const COleDateTimeSpan& dateSpan) const
{
        COleDateTimeSpan dateSpanTemp;

        // If either operand Null, result Null
        if (GetStatus() == null || dateSpan.GetStatus() == null)
        {
                dateSpanTemp.SetStatus(null);
                return dateSpanTemp;
        }

        // If either operand Invalid, result Invalid
        if (GetStatus() == invalid || dateSpan.GetStatus() == invalid)
        {
                dateSpanTemp.SetStatus(invalid);
                return dateSpanTemp;
        }

        // Add spans and validate within legal range
        dateSpanTemp.m_span = m_span + dateSpan.m_span;
        dateSpanTemp.CheckRange();

        return dateSpanTemp;
}

COleDateTimeSpan COleDateTimeSpan::operator-(const COleDateTimeSpan& dateSpan) const
{
        COleDateTimeSpan dateSpanTemp;

        // If either operand Null, result Null
        if (GetStatus() == null || dateSpan.GetStatus() == null)
        {
                dateSpanTemp.SetStatus(null);
                return dateSpanTemp;
        }

        // If either operand Invalid, result Invalid
        if (GetStatus() == invalid || dateSpan.GetStatus() == invalid)
        {
                dateSpanTemp.SetStatus(invalid);
                return dateSpanTemp;
        }

        // Subtract spans and validate within legal range
        dateSpanTemp.m_span = m_span - dateSpan.m_span;
        dateSpanTemp.CheckRange();

        return dateSpanTemp;
}

void COleDateTimeSpan::SetDateTimeSpan(
        long lDays, int nHours, int nMins, int nSecs)
{
        // Set date span by breaking into fractional days (all input ranges valid)
        m_span = lDays + ((double)nHours)/24 + ((double)nMins)/(24*60) +
                ((double)nSecs)/(24*60*60);

        SetStatus(valid);
}

CString COleDateTimeSpan::Format(LPCTSTR pFormat) const
{
        CString strSpan;
        struct tm tmTemp;

        // If null, return empty string
        if (GetStatus() == null)
                return strSpan;

        // If invalid, return DateTimeSpan resource string
        if (GetStatus() == invalid || !TmFromOleDate(m_span, tmTemp))
        {
                VERIFY(strSpan.LoadString(AFX_IDS_INVALID_DATETIMESPAN));
                return strSpan;
        }

        // Convert tm from afx internal format to standard format
        TmConvertToStandardFormat(tmTemp);

        // Fill in the buffer, disregard return value as it's not necessary
        LPTSTR lpszTemp = strSpan.GetBufferSetLength(MAX_TIME_BUFFER_SIZE);
        _tcsftime(lpszTemp, strSpan.GetLength(), pFormat, &tmTemp);
        strSpan.ReleaseBuffer();

        return strSpan;
}

CString COleDateTimeSpan::Format(UINT nFormatID) const
{
        CString strFormat;
        VERIFY(strFormat.LoadString(nFormatID) != 0);
        return Format(strFormat);
}

void COleDateTimeSpan::CheckRange()
{
        if(m_span < -MAX_DAYS_IN_SPAN || m_span > MAX_DAYS_IN_SPAN)
                SetStatus(invalid);
}

// serialization
#ifdef _DEBUG
CDumpContext& AFXAPI operator<<(CDumpContext& dc, COleDateTimeSpan dateSpanSrc)
{
        dc << "\nCOleDateTimeSpan Object:";
        dc << "\n\tm_status = " << (long)dateSpanSrc.m_status;

        COleVariant var(dateSpanSrc.m_span);
        var.ChangeType(VT_BSTR);

        return dc << "\n\tdateSpan = " << (LPCTSTR)var.bstrVal;
}
#endif // _DEBUG

#ifdef _with_archive_
CArchive& AFXAPI operator<<(CArchive& ar, COleDateTimeSpan dateSpanSrc)
{
        ar << (long)dateSpanSrc.m_status;
        return ar << dateSpanSrc.m_span;
}

CArchive& AFXAPI operator>>(CArchive& ar, COleDateTimeSpan& dateSpanSrc)
{
        ar >> (long&)dateSpanSrc.m_status;
        return ar >> dateSpanSrc.m_span;
}
#endif //_with_archive_

/////////////////////////////////////////////////////////////////////////////
// COleDateTime class HELPERS - implementation

BOOL OleDateFromTm(WORD wYear, WORD wMonth, WORD wDay,
        WORD wHour, WORD wMinute, WORD wSecond, DATE& dtDest)
{
        // Validate year and month (ignore day of week and milliseconds)
        if (wYear > 9999 || wMonth < 1 || wMonth > 12)
                return FALSE;

        //  Check for leap year and set the number of days in the month
        BOOL bLeapYear = ((wYear & 3) == 0) &&
                ((wYear % 100) != 0 || (wYear % 400) == 0);

        int nDaysInMonth =
                rgMonthDays[wMonth] - rgMonthDays[wMonth-1] +
                ((bLeapYear && wDay == 29 && wMonth == 2) ? 1 : 0);

        // Finish validating the date
        if (wDay < 1 || wDay > nDaysInMonth ||
                wHour > 23 || wMinute > 59 ||
                wSecond > 59)
        {
                return FALSE;
        }

        // Cache the date in days and time in fractional days
        long nDate;
        double dblTime;

        //It is a valid date; make Jan 1, 1AD be 1
        nDate = wYear*365L + wYear/4 - wYear/100 + wYear/400 +
                rgMonthDays[wMonth-1] + wDay;

        //  If leap year and it's before March, subtract 1:
        if (wMonth <= 2 && bLeapYear)
                --nDate;

        //  Offset so that 12/30/1899 is 0
        nDate -= 693959L;

        dblTime = (((long)wHour * 3600L) +  // hrs in seconds
                ((long)wMinute * 60L) +  // mins in seconds
                ((long)wSecond)) / 86400.;

        dtDest = (double) nDate + ((nDate >= 0) ? dblTime : -dblTime);

        return TRUE;
}

BOOL TmFromOleDate(DATE dtSrc, struct tm& tmDest)
{
        // The legal range does not actually span year 0 to 9999.
        if (dtSrc > MAX_DATE || dtSrc < MIN_DATE) // about year 100 to about 9999
                return FALSE;

        long nDays;             // Number of days since Dec. 30, 1899
        long nDaysAbsolute;     // Number of days since 1/1/0
        long nSecsInDay;        // Time in seconds since midnight
        long nMinutesInDay;     // Minutes in day

        long n400Years;         // Number of 400 year increments since 1/1/0
        long n400Century;       // Century within 400 year block (0,1,2 or 3)
        long n4Years;           // Number of 4 year increments since 1/1/0
        long n4Day;             // Day within 4 year block
                                                        //  (0 is 1/1/yr1, 1460 is 12/31/yr4)
        long n4Yr;              // Year within 4 year block (0,1,2 or 3)
        BOOL bLeap4 = TRUE;     // TRUE if 4 year block includes leap year

        double dblDate = dtSrc; // tempory serial date

        // If a valid date, then this conversion should not overflow
        nDays = (long)dblDate;

        // Round to the second
        dblDate += ((dtSrc > 0.0) ? HALF_SECOND : -HALF_SECOND);

        nDaysAbsolute = (long)dblDate + 693959L; // Add days from 1/1/0 to 12/30/1899

        dblDate = fabs(dblDate);
        nSecsInDay = (long)((dblDate - floor(dblDate)) * 86400.);

        // Calculate the day of week (sun=1, mon=2...)
        //   -1 because 1/1/0 is Sat.  +1 because we want 1-based
        tmDest.tm_wday = (int)((nDaysAbsolute - 1) % 7L) + 1;

        // Leap years every 4 yrs except centuries not multiples of 400.
        n400Years = (long)(nDaysAbsolute / 146097L);

        // Set nDaysAbsolute to day within 400-year block
        nDaysAbsolute %= 146097L;

        // -1 because first century has extra day
        n400Century = (long)((nDaysAbsolute - 1) / 36524L);

        // Non-leap century
        if (n400Century != 0)
        {
                // Set nDaysAbsolute to day within century
                nDaysAbsolute = (nDaysAbsolute - 1) % 36524L;

                // +1 because 1st 4 year increment has 1460 days
                n4Years = (long)((nDaysAbsolute + 1) / 1461L);

                if (n4Years != 0)
                        n4Day = (long)((nDaysAbsolute + 1) % 1461L);
                else
                {
                        bLeap4 = FALSE;
                        n4Day = (long)nDaysAbsolute;
                }
        }
        else
        {
                // Leap century - not special case!
                n4Years = (long)(nDaysAbsolute / 1461L);
                n4Day = (long)(nDaysAbsolute % 1461L);
        }

        if (bLeap4)
        {
                // -1 because first year has 366 days
                n4Yr = (n4Day - 1) / 365;

                if (n4Yr != 0)
                        n4Day = (n4Day - 1) % 365;
        }
        else
        {
                n4Yr = n4Day / 365;
                n4Day %= 365;
        }

        // n4Day is now 0-based day of year. Save 1-based day of year, year number
        tmDest.tm_yday = (int)n4Day + 1;
        tmDest.tm_year = n400Years * 400 + n400Century * 100 + n4Years * 4 + n4Yr;

        // Handle leap year: before, on, and after Feb. 29.
        if (n4Yr == 0 && bLeap4)
        {
                // Leap Year
                if (n4Day == 59)
                {
                        /* Feb. 29 */
                        tmDest.tm_mon = 2;
                        tmDest.tm_mday = 29;
                        goto DoTime;
                }

                // Pretend it's not a leap year for month/day comp.
                if (n4Day >= 60)
                        --n4Day;
        }

        // Make n4DaY a 1-based day of non-leap year and compute
        //  month/day for everything but Feb. 29.
        ++n4Day;

        // Month number always >= n/32, so save some loop time */
        for (tmDest.tm_mon = (n4Day >> 5) + 1;
                n4Day > rgMonthDays[tmDest.tm_mon]; tmDest.tm_mon++);

        tmDest.tm_mday = (int)(n4Day - rgMonthDays[tmDest.tm_mon-1]);

DoTime:
        if (nSecsInDay == 0)
                tmDest.tm_hour = tmDest.tm_min = tmDest.tm_sec = 0;
        else
        {
                tmDest.tm_sec = (int)nSecsInDay % 60L;
                nMinutesInDay = nSecsInDay / 60L;
                tmDest.tm_min = (int)nMinutesInDay % 60;
                tmDest.tm_hour = (int)nMinutesInDay / 60;
        }

        return TRUE;
}

void TmConvertToStandardFormat(struct tm& tmSrc)
{
        // Convert afx internal tm to format expected by runtimes (_tcsftime, etc)
        tmSrc.tm_year -= 1900;  // year is based on 1900
        tmSrc.tm_mon -= 1;      // month of year is 0-based
        tmSrc.tm_wday -= 1;     // day of week is 0-based
        tmSrc.tm_yday -= 1;     // day of year is 0-based
}

double DoubleFromDate(DATE dt)
{
        // No problem if positive
        if (dt >= 0)
                return dt;

        // If negative, must convert since negative dates not continuous
        // (examples: -1.25 to -.75, -1.50 to -.50, -1.75 to -.25)
        double temp = ceil(dt);
        return temp - (dt - temp);
}

DATE DateFromDouble(double dbl)
{
        // No problem if positive
        if (dbl >= 0)
                return dbl;

        // If negative, must convert since negative dates not continuous
        // (examples: -.75 to -1.25, -.50 to -1.50, -.25 to -1.75)
        double temp = floor(dbl); // dbl is now whole part
        return temp + (temp - dbl);
}

/////////////////////////////////////////////////////////////////////////////
// COleSafeArray class
COleSafeArray::COleSafeArray(const SAFEARRAY& saSrc, VARTYPE vtSrc)
{
        AfxSafeArrayInit(this);
        vt = vtSrc | VT_ARRAY;
        CheckError(::SafeArrayCopy((LPSAFEARRAY)&saSrc, &parray));
        m_dwDims = GetDim();
        m_dwElementSize = GetElemSize();
}

COleSafeArray::COleSafeArray(LPCSAFEARRAY pSrc, VARTYPE vtSrc)
{
        AfxSafeArrayInit(this);
        vt = vtSrc | VT_ARRAY;
        CheckError(::SafeArrayCopy((LPSAFEARRAY)pSrc, &parray));
        m_dwDims = GetDim();
        m_dwElementSize = GetElemSize();
}

COleSafeArray::COleSafeArray(const COleSafeArray& saSrc)
{
        AfxSafeArrayInit(this);
        *this = saSrc;
        m_dwDims = GetDim();
        m_dwElementSize = GetElemSize();
}

COleSafeArray::COleSafeArray(const VARIANT& varSrc)
{
        AfxSafeArrayInit(this);
        *this = varSrc;
        m_dwDims = GetDim();
        m_dwElementSize = GetElemSize();
}

COleSafeArray::COleSafeArray(LPCVARIANT pSrc)
{
        AfxSafeArrayInit(this);
        *this = pSrc;
        m_dwDims = GetDim();
        m_dwElementSize = GetElemSize();
}

// Operations
void COleSafeArray::Attach(VARIANT& varSrc)
{
        ASSERT(varSrc.vt & VT_ARRAY);

        // Free up previous safe array if necessary
        Clear();

        // give control of data to COleSafeArray
        memcpy(this, &varSrc, sizeof(varSrc));
        varSrc.vt = VT_EMPTY;
}

VARIANT COleSafeArray::Detach()
{
        VARIANT varResult = *this;
        vt = VT_EMPTY;
        return varResult;
}

// Assignment operators
COleSafeArray& COleSafeArray::operator=(const COleSafeArray& saSrc)
{
        ASSERT(saSrc.vt & VT_ARRAY);

        CheckError(::VariantCopy(this, (LPVARIANT)&saSrc));
        return *this;
}

COleSafeArray& COleSafeArray::operator=(const VARIANT& varSrc)
{
        ASSERT(varSrc.vt & VT_ARRAY);

        CheckError(::VariantCopy(this, (LPVARIANT)&varSrc));
        return *this;
}

COleSafeArray& COleSafeArray::operator=(LPCVARIANT pSrc)
{
        ASSERT(pSrc->vt & VT_ARRAY);

        CheckError(::VariantCopy(this, (LPVARIANT)pSrc));
        return *this;
}

COleSafeArray& COleSafeArray::operator=(const COleVariant& varSrc)
{
        ASSERT(varSrc.vt & VT_ARRAY);

        CheckError(::VariantCopy(this, (LPVARIANT)&varSrc));
        return *this;
}

// Comparison operators
BOOL COleSafeArray::operator==(const SAFEARRAY& saSrc) const
{
        return CompareSafeArrays(parray, (LPSAFEARRAY)&saSrc);
}

BOOL COleSafeArray::operator==(LPCSAFEARRAY pSrc) const
{
        return CompareSafeArrays(parray, (LPSAFEARRAY)pSrc);
}

BOOL COleSafeArray::operator==(const COleSafeArray& saSrc) const
{
        if (vt != saSrc.vt)
                return FALSE;

        return CompareSafeArrays(parray, saSrc.parray);
}

BOOL COleSafeArray::operator==(const VARIANT& varSrc) const
{
        if (vt != varSrc.vt)
                return FALSE;

        return CompareSafeArrays(parray, varSrc.parray);
}

BOOL COleSafeArray::operator==(LPCVARIANT pSrc) const
{
        if (vt != pSrc->vt)
                return FALSE;

        return CompareSafeArrays(parray, pSrc->parray);
}

BOOL COleSafeArray::operator==(const COleVariant& varSrc) const
{
        if (vt != varSrc.vt)
                return FALSE;

        return CompareSafeArrays(parray, varSrc.parray);
}

void COleSafeArray::CreateOneDim(VARTYPE vtSrc, DWORD dwElements,
        void* pvSrcData, long nLBound)
{
        ASSERT(dwElements > 0);

        // Setup the bounds and create the array
        SAFEARRAYBOUND rgsabound;
        rgsabound.cElements = dwElements;
        rgsabound.lLbound = nLBound;
        Create(vtSrc, 1, &rgsabound);

        // Copy over the data if neccessary
        if (pvSrcData != NULL)
        {
                void* pvDestData;
                AccessData(&pvDestData);
                memcpy(pvDestData, pvSrcData, GetElemSize() * dwElements);
                UnaccessData();
        }
}

DWORD COleSafeArray::GetOneDimSize()
{
        ASSERT(GetDim() == 1);

        long nUBound, nLBound;

        GetUBound(1, &nUBound);
        GetLBound(1, &nLBound);

        return nUBound + 1 - nLBound;
}

void COleSafeArray::ResizeOneDim(DWORD dwElements)
{
        ASSERT(GetDim() == 1);

        SAFEARRAYBOUND rgsabound;

        rgsabound.cElements = dwElements;
        rgsabound.lLbound = 0;

        Redim(&rgsabound);
}

void COleSafeArray::Create(VARTYPE vtSrc, DWORD dwDims, DWORD* rgElements)
{
        ASSERT(rgElements != NULL);

        // Allocate and fill proxy array of bounds (with lower bound of zero)
        SAFEARRAYBOUND* rgsaBounds = new SAFEARRAYBOUND[dwDims];

        for (DWORD dwIndex = 0; dwIndex < dwDims; dwIndex++)
        {
                // Assume lower bound is 0 and fill in element count
                rgsaBounds[dwIndex].lLbound = 0;
                rgsaBounds[dwIndex].cElements = rgElements[dwIndex];
        }

        TRY
        {
                Create(vtSrc, dwDims, rgsaBounds);
        }
        CATCH_ALL(e)
        {
                // Must free up memory
                delete [] rgsaBounds;
                THROW_LAST();
        }
        END_CATCH_ALL

        delete [] rgsaBounds;
}

void COleSafeArray::Create(VARTYPE vtSrc, DWORD dwDims, SAFEARRAYBOUND* rgsabound)
{
        ASSERT(dwDims > 0);
        ASSERT(rgsabound != NULL);

        // Validate the VARTYPE for SafeArrayCreate call
        ASSERT(!(vtSrc & VT_ARRAY));
        ASSERT(!(vtSrc & VT_BYREF));
        ASSERT(!(vtSrc & VT_VECTOR));
        ASSERT(vtSrc != VT_EMPTY);
        ASSERT(vtSrc != VT_NULL);

        // Free up old safe array if necessary
        Clear();

        parray = ::SafeArrayCreate(vtSrc, dwDims, rgsabound);

        if (parray == NULL)
                AfxThrowMemoryException();

        vt = unsigned short(vtSrc | VT_ARRAY);
        m_dwDims = dwDims;
        m_dwElementSize = GetElemSize();
}

void COleSafeArray::AccessData(void** ppvData)
{
        CheckError(::SafeArrayAccessData(parray, ppvData));
}

void COleSafeArray::UnaccessData()
{
        CheckError(::SafeArrayUnaccessData(parray));
}

void COleSafeArray::AllocData()
{
        CheckError(::SafeArrayAllocData(parray));
}

void COleSafeArray::AllocDescriptor(DWORD dwDims)
{
        CheckError(::SafeArrayAllocDescriptor(dwDims, &parray));
}

void COleSafeArray::Copy(LPSAFEARRAY* ppsa)
{
        CheckError(::SafeArrayCopy(parray, ppsa));
}

void COleSafeArray::GetLBound(DWORD dwDim, long* pLbound)
{
        CheckError(::SafeArrayGetLBound(parray, dwDim, pLbound));
}

void COleSafeArray::GetUBound(DWORD dwDim, long* pUbound)
{
        CheckError(::SafeArrayGetUBound(parray, dwDim, pUbound));
}

void COleSafeArray::GetElement(long* rgIndices, void* pvData)
{
        CheckError(::SafeArrayGetElement(parray, rgIndices, pvData));
}

void COleSafeArray::PtrOfIndex(long* rgIndices, void** ppvData)
{
        CheckError(::SafeArrayPtrOfIndex(parray, rgIndices, ppvData));
}

void COleSafeArray::PutElement(long* rgIndices, void* pvData)
{
        CheckError(::SafeArrayPutElement(parray, rgIndices, pvData));
}

void COleSafeArray::Redim(SAFEARRAYBOUND* psaboundNew)
{
        CheckError(::SafeArrayRedim(parray, psaboundNew));
}

void COleSafeArray::Lock()
{
        CheckError(::SafeArrayLock(parray));
}

void COleSafeArray::Unlock()
{
        CheckError(::SafeArrayUnlock(parray));
}

void COleSafeArray::Destroy()
{
        CheckError(::SafeArrayDestroy(parray));
}

void COleSafeArray::DestroyData()
{
        CheckError(::SafeArrayDestroyData(parray));
}

void COleSafeArray::DestroyDescriptor()
{
        CheckError(::SafeArrayDestroyDescriptor(parray));
}

///////////////////////////////////////////////////////////////////////////////
// COleSafeArray Helpers
void AFXAPI AfxSafeArrayInit(COleSafeArray* psa)
{
        memset(psa, 0, sizeof(*psa));
}

/////////////////////////////////////////////////////////////////////////////
// Simple field formatting to text item - see dlgdata.cpp for base types
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, COleDateTime& value)
{
        HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
        if (pDX->m_bSaveAndValidate)
        {
                int nLen = ::GetWindowTextLength(hWndCtrl);
                CString strTemp;

                ::GetWindowText(hWndCtrl, strTemp.GetBufferSetLength(nLen), nLen+1);
                strTemp.ReleaseBuffer();

                if (!value.ParseDateTime(strTemp))  // throws exception
                {
                        // Can't convert string to datetime
                        AfxMessageBox(AFX_IDP_PARSE_DATETIME);
                        pDX->Fail();    // throws exception
                }
        }
        else
        {
                CString strTemp = value.Format();
                AfxSetWindowText(hWndCtrl, strTemp);
        }
}

void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, COleCurrency& value)
{
        HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
        if (pDX->m_bSaveAndValidate)
        {
                int nLen = ::GetWindowTextLength(hWndCtrl);
                CString strTemp;

                ::GetWindowText(hWndCtrl, strTemp.GetBufferSetLength(nLen), nLen+1);
                strTemp.ReleaseBuffer();

                if (!value.ParseCurrency(strTemp))  // throws exception
                {
                        // Can't convert string to currency
                        AfxMessageBox(AFX_IDP_PARSE_CURRENCY);
                        pDX->Fail();    // throws exception
                }
        }
        else
        {
                CString strTemp = value.Format();
                AfxSetWindowText(hWndCtrl, strTemp);
        }
}

/////////////////////////////////////////////////////////////////////////////
#endif //_CURRENCY_ALSO_

//+---------------------------------------------------------------------------
//
//  Method:     COleVariant::Save
//
//  Synopsis:   saves a variant to a stream
//
//  Arguments:  [pStm] --
//              [fClearDirty] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:      BUGBUG:NOT COMPLETE!
//
//----------------------------------------------------------------------------
HRESULT COleVariant::Save(IStream *pStm, BOOL fClearDirty)
{
    LPVARIANT pSrc = (LPVARIANT)this;
    HRESULT hr = NOERROR;
    ULONG cbSaved;
    COleVariant CVar;

    if (pSrc->vt & VT_BYREF || pSrc->vt & VT_ARRAY)
    {
        // No support for VT_BYREF & VT_ARRAY
        pSrc = (LPVARIANT)&CVar;
    }

    // write the variant
    hr = pStm->Write(pSrc, sizeof(VARIANT), &cbSaved);
    TransAssert(( sizeof(VARIANT) == cbSaved));
    if (hr == NOERROR)
    {
        switch (pSrc->vt)
        {
        case VT_DISPATCH:
            TransAssert((FALSE));
            hr = E_FAIL;
            break;

        case VT_UNKNOWN:
            {
                ULONG cbInterface = 0;
                IUnknown *pUnk = pSrc->punkVal;
                
                hr = CoGetMarshalSizeMax(&cbInterface, IID_IUnknown, pUnk, MSHCTX_LOCAL, 0, MSHLFLAGS_NORMAL);

                if (hr == S_OK)
                {   
                    // write the size of 
                    hr = pStm->Write(&cbInterface, sizeof(ULONG), &cbSaved);
                    // need to marshal table strong
                    hr = CoMarshalInterface(pStm, IID_IUnknown, pUnk, MSHCTX_LOCAL, 0, MSHLFLAGS_NORMAL);
                }

            }
            break;
            

        default:
        case VT_EMPTY:
        case VT_NULL:
            // do nothing
            break;

        case VT_BSTR:
            {
                DWORD nLen = SysStringByteLen(pSrc->bstrVal);
                hr = pStm->Write(&nLen, sizeof(DWORD), &cbSaved);
                if (   (hr == NOERROR)
                    && (nLen > 0))
                {
                    hr = pStm->Write(pSrc->bstrVal, nLen * sizeof(BYTE), &cbSaved);
                }
            }
            break;


    #ifdef _unused_
        case VT_BOOL:
            hr = pStm->Write((WORD)V_BOOL(pSrc), sizeof(WORD), &cbSaved);
            break;

        case VT_UI1:
            hr = pStm->Write(pSrc->bVal, sizeof(WORD), &cbSaved);
            break;

        case VT_I2:
            hr = pStm->Write((WORD)pSrc->iVal, sizeof(WORD), &cbSaved);
            break;

        case VT_I4:
            hr = pStm->Write(pSrc->lVal, sizeof(DWORD), &cbSaved);
            break;

        case VT_CY:
            hr = pStm->Write(pSrc->cyVal.Lo, sizeof(WORD), &cbSaved);
            hr = pStm->Write(pSrc->cyVal.Hi, sizeof(WORD), &cbSaved);
            break;

        case VT_R4:
            hr = pStm->Write(pSrc->fltVal, sizeof(WORD), &cbSaved);
            break;

        case VT_R8:
            hr = pStm->Write(pSrc->dblVal, sizeof(DOUBLE), &cbSaved);
            break;

        case VT_DATE:
            hr = pStm->Write(pSrc->date;
            break;

        case VT_ERROR:
            hr = pStm->Write(pSrc->scode;
            break;
    #endif //_unused_
        }
    }


    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COleVariant::Load
//
//  Synopsis:   loads a variant from a stream
//
//  Arguments:  [pStm] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:      BUGBUG:NOT COMPLETE!
//
//----------------------------------------------------------------------------
HRESULT COleVariant::Load(IStream *pStm)
{
    LPVARIANT pSrc = (LPVARIANT)this;
    HRESULT hr = NOERROR;
    ULONG cbSaved;
    COleVariant CVar;

    // Read the variant
    hr = pStm->Read(pSrc, sizeof(VARIANT), &cbSaved);
    TransAssert(( sizeof(VARIANT) == cbSaved));

    if (pSrc->vt & VT_BYREF || pSrc->vt & VT_ARRAY)
    {
        // No support for VT_BYREF & VT_ARRAY
        pSrc = (LPVARIANT)&CVar;
    }

    if (hr == NOERROR)
    {
        switch (pSrc->vt)
        {
        case VT_DISPATCH:
            TransAssert((FALSE));
            hr = E_FAIL;
            break;


        case VT_UNKNOWN:
            {
                ULONG cbInterface = 0;
                IUnknown *pUnk = 0;
                
                // write the size of 
                hr = pStm->Read(&cbInterface, sizeof(ULONG), &cbSaved);

                if (   (hr == S_OK)
                    && cbInterface)
                {

                    hr = CoUnmarshalInterface(pStm, IID_IUnknown, (void **) &pUnk);
                    if(hr == S_OK)
                    {
                        pSrc->punkVal = pUnk;
                    }
                    else
                    {
                        pSrc->punkVal = 0;
                    }
                }
                else
                {
                    pSrc->punkVal = 0;
                }
                hr = NOERROR;
            }
            break;


        default:
        case VT_EMPTY:
        case VT_NULL:
            // do nothing
            break;

        case VT_BSTR:
            {
                DWORD nLen = 0;
                hr = pStm->Read(&nLen, sizeof(DWORD), &cbSaved);

                if (nLen > 0)
                {
                    pSrc->bstrVal = SysAllocStringByteLen(NULL, nLen);

                    if (pSrc->bstrVal)
                    {
                        hr = pStm->Read(pSrc->bstrVal, nLen * sizeof(BYTE), &cbSaved);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            break;


    #ifdef _unused_
        case VT_BOOL:
            hr = pStm->Read((WORD)V_BOOL(pSrc), sizeof(WORD), &cbSaved);
            break;

        case VT_UI1:
            hr = pStm->Read(pSrc->bVal, sizeof(WORD), &cbSaved);
            break;

        case VT_I2:
            hr = pStm->Read((WORD)pSrc->iVal, sizeof(WORD), &cbSaved);
            break;

        case VT_I4:
            hr = pStm->Read(pSrc->lVal, sizeof(DWORD), &cbSaved);
            break;

        case VT_CY:
            hr = pStm->Read(pSrc->cyVal.Lo, sizeof(WORD), &cbSaved);
            hr = pStm->Read(pSrc->cyVal.Hi, sizeof(WORD), &cbSaved);
            break;

        case VT_R4:
            hr = pStm->Read(pSrc->fltVal, sizeof(WORD), &cbSaved);
            break;

        case VT_R8:
            hr = pStm->Read(pSrc->dblVal, sizeof(DOUBLE), &cbSaved);
            break;

        case VT_DATE:
            hr = pStm->Read(pSrc->date;
            break;

        case VT_ERROR:
            hr = pStm->Read(pSrc->scode;
            break;
    #endif //_unused_
        }
    }

    return hr;
}




