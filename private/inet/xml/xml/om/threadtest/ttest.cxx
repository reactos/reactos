/*
 * @(#)omtest.cxx 1.0 12/12/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 * This program tests the MSXML COM interface to the object manager (OM).
 * It's based on code from Julian Jiggins (omtest2) and modified by Dan Tripp.
 * This code parses an XML document then dumps the object model to a file.
 * Rather than formatting the output in XML syntax, the tree is printed in
 * outline form with the element names and types, text contents, etc.  In
 * addition, various return values from COM interface calls are printed,
 * and certain conditions are noted.  That way if any of this changes it
 * will be visible in the output of this program.
 *
 * Normal use of this program is to generate an output file and compare it
 * with a previously generated file.
 *
 * If you look at the output, most of what you see will be obvious.  Values
 * that may not be obvious are non S_OK HRESULT return codes printed in < >
 * brackets, and text checksums printed in parentheses.  The text checksums
 * are useful in making sure the same text continues to be returned from
 * get_text at all levels in the tree.
 */

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <urlmon.h>
#include <hlink.h>
#include <dispex.h>
#include "mshtml.h"
#include "msxml.h"

#include <winnls.h>

#define ASSERT(x)  if(!(x)) DebugBreak()
#define CHECK_ERROR(cond, err) if (!(cond)) {pszErr=(err); goto done;}
#define SAFERELEASE(p) if (p) {(p)->Release(); p = NULL;} else ;

FILE* out = stdout;
int errorFormat = 0;
bool testEnum = false;

DWORD WINAPI CreateParserThread(void * pv);

void TestEnumInterface(IXMLElementCollection * pChildren, int indent);

#define WALK_ELEMENT_COLLECTION(pCollection, pDispItem) \
    {\
        long length;\
        \
        if (SUCCEEDED(pChildren->get_length(&length)) && length > 0)\
        {\
            VARIANT vIndex, vEmpty;\
            vIndex.vt = VT_I4;\
            vEmpty.vt = VT_EMPTY;\
                                 \
            for (long i=0; i<length; i++)\
            {\
                vIndex.lVal = i;\
                IDispatch *pDispItem = NULL;\
                if (SUCCEEDED(pCollection->item(vIndex, vEmpty, &pDispItem)))\
                {

#define END_WALK_ELEMENT_COLLECTION(pDispItem) \
                    pDispItem->Release();\
                }\
            }\
        }\
    }


void ToUpper(WCHAR* str)
{
    if (!str) return;

    for ( ; *str; str++)
        if (*str >= L'a' && *str <= L'z')
            *str = *str - L'a' + L'A';
}

int CheckSum(WCHAR* str)
{
    if (!str) return 0;

    int result = 1;
    for ( ; *str; str++)
    {
        int highbit = (result < 0);
        result <<= 1;
        result |= highbit;
        result ^= *str;
    }
    return result;
}

//
// Dump an element attribute member if present
//
void DumpAttrib(IXMLElement *pElem, BSTR bstrAttribName)
{
    VARIANT vProp;
    
    VariantInit(&vProp);

    if (SUCCEEDED(pElem->getAttribute(bstrAttribName, &vProp)))
    {
        if (vProp.vt == VT_BSTR)
        {
            fprintf(out, " %S=\"%S\"", bstrAttribName, vProp.bstrVal);
        }
        VariantClear(&vProp);
    }
}


static void printhr(HRESULT hr)
{
    if (hr != 0)
        fprintf(out, "<%lx>", hr);
}

static void dumpTree(
    IXMLElement *pElem, // the XML element to dump
    int indent          // indentation level
)
{
    for (int i = 0; i < indent-1; i++)
        fprintf(out, "|   ");

    if (indent > 0)
        fprintf(out, "|---");

    HRESULT hr;
    bool dumpText = false;
    bool dumpTagName = true;
    bool dumpAttributes = true;

    XMLELEM_TYPE xmlElemType;
    hr = pElem->get_type((long *)&xmlElemType);
    printhr(hr);
    if (!SUCCEEDED(hr))
        return;

    switch (xmlElemType) {
    case XMLELEMTYPE_TEXT:
        fprintf(out, "TEXT");
        dumpText = true;
        break;
    case XMLELEMTYPE_COMMENT:
        fprintf(out, "COMMENT");
        dumpText = true;
        break;
    case XMLELEMTYPE_DOCUMENT:
        fprintf(out, "DOCUMENT");
        break;
    case XMLELEMTYPE_DTD:
        fprintf(out, "DOCTYPE");
        break;
    case XMLELEMTYPE_ELEMENT:
        fprintf(out, "ELEMENT");
        break;
    case XMLELEMTYPE_PI:
        fprintf(out, "PI");
        dumpText = true;
        break;
    case XMLELEMTYPE_OTHER:
        fprintf(out, "OTHER");
        dumpText = true;
        break;
    }
        
//    if (dumpTagName)
    if (true) 
    { 
        BSTR bstrTagName = NULL;
        hr = pElem->get_tagName(&bstrTagName);
        printhr(hr);
        if (SUCCEEDED(hr))
        {
            fprintf(out, " '%S'", bstrTagName);
            SysFreeString(bstrTagName);
        }
        else
        {
            fprintf(out, "[NoTag]");
        }
    }
//    if (dumpAttributes)
    if (true) 
    {
        // Some CDF/OSD attribute names, in various upper/lower case combinations
        DumpAttrib(pElem, L"value");    // case insensitive
        DumpAttrib(pElem, L"HREF");
        DumpAttrib(pElem, L"Hour");
        DumpAttrib(pElem, L"Day");
        DumpAttrib(pElem, L"TimeZone");
        DumpAttrib(pElem, L"IsClonable");
        DumpAttrib(pElem, L"Type");
        DumpAttrib(pElem, L"style");
        DumpAttrib(pElem, L"charset");
        DumpAttrib(pElem, L"title");
        DumpAttrib(pElem, L"show");
        DumpAttrib(pElem, L"method");
        DumpAttrib(pElem, L"authenticate");
        DumpAttrib(pElem, L"precache");
        DumpAttrib(pElem, L"self");
        DumpAttrib(pElem, L"BASE");
        DumpAttrib(pElem, L"XML-SPACE");
        DumpAttrib(pElem, L"NaMe");
        DumpAttrib(pElem, L"VERSION");
        DumpAttrib(pElem, L"action");
    }

    if (true)
    {
        WCHAR dummy[] = L"<<bogus!>>";
        WCHAR empty[] = L"";
        BSTR bstrContent = dummy;
        hr = pElem->get_text(&bstrContent);
//        printhr(hr);
        if (hr == S_OK && bstrContent == dummy)
        {
            // Diff: bug in old MSXML.  Returns S_OK but no string.
            bstrContent = empty;
        }
        else if (hr == S_FALSE && bstrContent == NULL)
        {
            // Diff: old MSXML returned S_FALSE for container with no
            // text in children.
            hr = S_OK;
            bstrContent = empty;
        }

        fprintf(out, "(%d)", CheckSum(bstrContent));

        if (dumpText)
        {
            fprintf(out, "\"%S\"", bstrContent);
        }

        if (SUCCEEDED(hr) && bstrContent && bstrContent != empty)
        {
            SysFreeString(bstrContent);
        }
    }


    IXMLElementCollection * pChildren;
    if (SUCCEEDED(hr = pElem->get_children(&pChildren)))
    {
        if (pChildren)
        {
            printhr(hr);
            fprintf(out, "\n");

            if (testEnum)
            {
                TestEnumInterface(pChildren, indent);
            }
            else
            {
                WALK_ELEMENT_COLLECTION(pChildren, pDisp)
                {
                    IXMLElement * pChild;
                    if (SUCCEEDED(pDisp->QueryInterface(
                                             IID_IXMLElement, (void **)&pChild)))
                    {
                        dumpTree(pChild, indent+1);
                        pChild->Release();
                    }
                }
                END_WALK_ELEMENT_COLLECTION(pDisp);
            }
            pChildren->Release();
        }
        else if (hr == S_FALSE)
        {
            // Diff:
            // known incompatibility when there are no children.
            // old: return S_FALSE and null child collection pointer
            // new: return S_OK and empty child collection
            fprintf(out, "\n");
        }
        else
        {
            printhr(hr);
            fprintf(out, "[NoKids2]\n");
        }
    }
    else
    {
        printhr(hr);
        fprintf(out, "[NoKids1]\n");
    }
}

void TestEnumInterface(IXMLElementCollection * pChildren, int indent)
{
    IEnumVARIANT * pEnum;
    HRESULT hr = S_OK;

    if (SUCCEEDED(pChildren->get__newEnum((IUnknown **)&pEnum)))
    {
        ULONG l1 = 1, l2 = 0, l = 0;
        VARIANT p[20];

        do {
            hr = pEnum->Next(l1, (VARIANT *)p, &l2);
            if (hr != S_OK)
                break;
            IDispatch * d = V_DISPATCH(&(p[0]));
            IXMLElement * pChild;
            hr = d->QueryInterface(IID_IXMLElement, (void **)&pChild);
            if (hr == S_OK)
            {
                l++;
                dumpTree(pChild, indent+1);
                pChild->Release();
            }
            d->Release();
        } while (hr == S_OK);

        //further test
        for (int i = 2; i< 21; i++)
        {
            bool gf = false;
            l1 = 0;
            hr = pEnum->Reset();
            do {
                hr = pEnum->Next(i, (VARIANT *)p, &l2);
                if (FAILED(hr))
                    break;
                l1 += l2;
                if (hr == S_FALSE)
                {
                     gf = true;
                     break;
                }
                for (UINT j=0; j< l2; j++)
                {
                    IDispatch * d = V_DISPATCH(&(p[j]));
                    d->Release();
                }
            } while (hr == S_OK);
            if (l1 != l || !gf)
                fprintf(stderr, "Something is wrong!!\7");
        }
    }
}

BOOL TestDocument(IXMLDocument* pDoc)
{
    HRESULT hr;
    IXMLElement            *pElem = NULL;
    WCHAR                  dummy[] = L"<<bogus!>>";
    BSTR                   bstrVal = dummy;
    BOOL result = TRUE; 

    long fReady;
    pDoc->get_readyState(&fReady);
    if (fReady == READYSTATE_UNINITIALIZED)
        return FALSE;

    // First try to get the root element.  Do this before anything else
    // because of the way the old MSXML handles some errors.  In some cases
    // put_URL returns S_OK even though there was an error.  However, 
    // get_root will fail in these cases.  We check this first to avoid
    // printing out errors when we try to get the version, encoding, etc.

    pElem = NULL;
    hr = pDoc->get_root(&pElem);
    if (FAILED(hr) || pElem == NULL)
    {
        result = FALSE;
        goto done;
    }

    // Dump version

    fprintf(out, "XMLDECL:");
    hr = pDoc->get_version(&bstrVal);
    // Diff: Old MSXML returned S_FALSE if no <?XML in file.
    if (hr == S_FALSE)
    {
        fprintf(out, " version='1.0'");
    }
    else
    {
        printhr(hr);
        fprintf(out, " version='%S'", bstrVal);
    }
    if (SUCCEEDED(hr))
    {
        SysFreeString(bstrVal);
    }

    // Dump encoding (charset)

    bstrVal = dummy;
    hr = pDoc->get_charset(&bstrVal);
    printhr(hr);
    fprintf(out, " encoding='%S'", bstrVal);
    if(SUCCEEDED(hr))
    {
        SysFreeString(bstrVal);
    }
    fprintf(out, "\n");

    // Dump doctype

    fprintf(out, "DOCTYPE:");

    bstrVal = dummy;
    hr = pDoc->get_doctype(&bstrVal);
    printhr(hr);
    ToUpper(bstrVal);
    fprintf(out, " %S", bstrVal); 
    if(SUCCEEDED(hr))
    { 
        SysFreeString(bstrVal);
    }
    fprintf(out, "\n");

    // Dump the root element and all its children
    
    fprintf(out, "\n");
    dumpTree(pElem, 0);

    // Check get_text on the root element.

    fprintf(out, "\n\nRoot get_text:");
    bstrVal = dummy;
    hr = pElem->get_text(&bstrVal);
    // Diff: no text returns S_FALSE and NULL in old parser, empty
    // string in new one.
    if ((hr == S_OK && bstrVal && *bstrVal == 0) ||     // new msxml
        (hr == S_FALSE && bstrVal == NULL) ||           // old msxml
        (hr == S_OK && bstrVal == dummy))               // old msxml
    {
        fprintf(out, "no text\n");
    }
    else
    {
        printhr(hr);
        fprintf(out, "checksum=%d\n", CheckSum(bstrVal));
    }
    if(SUCCEEDED(hr))
    {
        SysFreeString(bstrVal);
    }

done:
    SAFERELEASE(pElem);
    return result;
}

void ParseXML(char *fname, int loops, BOOL quiet, BOOL pause)
{
    PSTR pszErr = NULL;
    IXMLDocument           *pDoc = NULL;
    IStream                *pStm = NULL;
    IPersistStreamInit     *pPSI = NULL;
    WCHAR                  wszFName[MAX_PATH];
    WCHAR                  buf[MAX_PATH];
    WCHAR                  *pwszURL=NULL;
    BSTR                   pBURL=NULL;
    long  min = 0, max = 0, sum=0;
    long i;

    HRESULT hr = S_OK;

    MultiByteToWideChar(CP_ACP, 0, fname, -1, wszFName, MAX_PATH);

    //
    // HACK if passed in a file name expand it if it doesn't look like an URL
    //
    if (strncmp(fname, "http://", 7) == 0)
    {
        pwszURL = wszFName;
    }
    else
    {
        DWORD count = GetFullPathName(wszFName, MAX_PATH, buf, NULL);
        if (count == 0)
        {
            // hack for Win95, since there is no Unicode GetFullPathName.
            char buf2[MAX_PATH];
            count = GetFullPathNameA(fname, MAX_PATH, buf2, NULL);
            MultiByteToWideChar(CP_ACP, 0, buf2, -1, buf, MAX_PATH);
        }
        pwszURL = buf;
    }

//    ASSERT(SUCCEEDED(hr));

    for(i = 0; i < loops; i++)
    {
        //
        // Create an empty XML document
        //
        hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IXMLDocument, (void**)&pDoc);

        CHECK_ERROR (pDoc, "CoCreateInstance Failed");

        LARGE_INTEGER perfStart, perfEnd, perfFreq;
        QueryPerformanceCounter(&perfStart);

        //
        // Synchronously create a stream on an URL
        //
        hr = URLOpenBlockingStream(0, pwszURL, &pStm, 0,0);    
        CHECK_ERROR(SUCCEEDED(hr) && pStm, "Couldn't open stream on URL")
    
        //
        // Get the IPersistStreamInit interface to the XML doc
        //
        hr = pDoc->QueryInterface(IID_IPersistStreamInit, (void **)&pPSI);
        CHECK_ERROR(SUCCEEDED(hr), "QI for IPersistStreamInit failed");
//
// BUGBUG:: need this to make the output the same as those in testoutput folder
//
        fprintf(out, "put_URL:\n");

        //
        // Init the XML doc from the stream
        //

        hr = pPSI->Load(pStm);
    
        QueryPerformanceCounter(&perfEnd);
        QueryPerformanceFrequency(&perfFreq);

        if (loops > 0)
        {
            __int64 i64Time = *((__int64 *) &perfEnd) - *((__int64 *) &perfStart);
            __int64 i64perf = *((__int64 *) &perfFreq);
            i64Time = i64Time * 1000 / i64perf;
            long t = (long)i64Time;
            if (t < min || min == 0)
                min = t;
            if (t > max)
                max = t;

            sum += t;
            fprintf(stderr, "Loaded in %ld milliseconds\n", t);
        }

        if (pause)
        {
            fprintf(stderr, "Press any key to continue...");
            getchar();
        }

//    pwszURL = (WCHAR *)LocalAlloc(LMEM_FIXED, ((sizeof(WCHAR))*(strlen(pszURL) + 2)));
//    CHECK_ERROR(pwszURL, "Mem Alloc Failure");

//    hr = MyStrToOleStrN(pwszURL, (strlen(pszURL) + 1), pszURL);
//    CHECK_ERROR(SUCCEEDED(hr), "Failed to convert to UNICODE");

//    pBURL = SysAllocString(pwszURL);
//    CHECK_ERROR(pBURL, "Mem Alloc Failure");
//        LocalFree(pwszURL);

    // Load the document

//    hr = pDoc->put_URL(pBURL);

        if (FAILED(hr))
            goto error;

        if (! quiet)
            if (! TestDocument(pDoc))
                goto error;

        if (i < loops -1 )
        {
            SAFERELEASE(pPSI);
            SAFERELEASE(pStm);
            SAFERELEASE(pDoc);
        }
    }

    if (loops > 0)
    {
        fprintf(stderr, "min=%ld, max=%ld, avg=%ld\n",
            min, max, (sum/loops));

    }

//        fprintf(out, "put_URL:");
//    printhr(hr);              // Diff: see comment below about put_URL.
//    fprintf(out, "\n");

    goto done;

error:

    {
        ASSERT(pDoc);

        //
        // Failed to parse stream, output error information
        //
        IXMLError *pXMLError = NULL ;
        XML_ERROR xmle;

        hr = pDoc->QueryInterface(IID_IXMLError, (void **)&pXMLError);
        CHECK_ERROR(SUCCEEDED(hr), "Couldn't get IXMLError");

        ASSERT(pXMLError);

        hr = pXMLError->GetErrorInfo(&xmle);
        SAFERELEASE(pXMLError);
        CHECK_ERROR(SUCCEEDED(hr), "GetErrorInfo Failed");

        switch (errorFormat)
        {
        case 1:
            fprintf(out, "Error on line %d, char %d.\nFound %S while expecting %S\r\n",
                    xmle._nLine,
                    xmle._ich,
                    xmle._pszFound,
                    xmle._pszExpected);
            break;
        case 2:

            if (*xmle._pszFound)
            {
                fprintf(out, "Error on line %d, char %d.\nFound %S while expecting %S\r\n",
                        xmle._nLine,
                        xmle._ich,
                        xmle._pszFound,
                        xmle._pszExpected);
            }
            else
            {
                fprintf(out, "Error on line %d, char %d.\n%S\n",
                        xmle._nLine,
                        xmle._ich,
                        xmle._pszExpected);
            }
            break;

        case 3:
            if (xmle._nLine > 0 || xmle._ich > 0)
                fprintf(out, "Error on line %d, character %d\n", xmle._nLine, xmle._ich);
            else
                fprintf(out, "Error -- 'expected'=%S\n", xmle._pszExpected);

        default:
            fprintf(out, "Error on line %d\n", xmle._nLine);
            break;

        break;
        }
        SysFreeString(xmle._pszFound);
        SysFreeString(xmle._pszExpected);
        SysFreeString(xmle._pchBuf);
    }
                         
done: // Clean up.
    //
    // Release any used interfaces
    //
    SAFERELEASE(pPSI);
    SAFERELEASE(pStm);
    SAFERELEASE(pDoc);
    SysFreeString(pBURL);
    if (pszErr)
        fprintf (stderr, "%s, last error %d\n", pszErr, GetLastError());
}

#define MAX_THREADS 16

struct Args {
    char *  pchFile;
    CRITICAL_SECTION *pcs;
    BOOL    bQuiet;
    int     iCount;
    BOOL    bPause;
};

int _cdecl 
main(int argc, char *argv[])
{
    int i;
    int cThreads = 0;         
    HANDLE  hThread[MAX_THREADS];
    DWORD   idThread[MAX_THREADS];
    Args    a[MAX_THREADS];
    CRITICAL_SECTION    cs;

    InitializeCriticalSection(&cs);

    a[0].pcs = &cs;
    a[0].bQuiet = FALSE;
    a[0].iCount = 1;
    a[0].bPause = FALSE;

    for (i = 1; i < argc; i++)
    {
        if (*argv[i] == '-')
        {
            switch(toupper(argv[i][1]))
            {
            case 'O':
                if (out != stdout) fclose(out);
                out = fopen(argv[++i], "w");   // assume the arg is present!
                break;

            case 'E':
                errorFormat = argv[i][2] - '0';
                break;

            case 'G':
                testEnum = true;
                break;

            case 'T':
                a[0].iCount = atoi(argv[++i]);
                break;

            case 'Q':
                a[0].bQuiet = TRUE;
                break;

            case 'P':
                a[0].bPause = TRUE;
                break;

            default:
                fprintf(stderr, "omtest [-d] [-eN] [-g] [-q] [-t n] [-o filename] [-h] filename1 [filename2] [filename3] ...\n");
                fprintf(stderr, "-o writes output to named file\n");
                fprintf(stderr, "-e selects error reporting format\n");
                fprintf(stderr, "-g tests element collection get__newEnum\n");
                fprintf(stderr, "-h prints this message\n");
                fprintf(stderr, "-q stops verbose output\n");
                fprintf(stderr, "-t n tells it to time n-loads of the given filename\n");
                fprintf(stderr, "Example: test hamlet.xml channel.xml foo.xml\n");
                fprintf(stderr, "Parses hamlet.xml channel.xml and foo.xml using threads and dumps the parse tree\n");
            }
        }
        else if (cThreads < MAX_THREADS)
        {
            a[cThreads++].pchFile = argv[i];
            a[cThreads].pcs = &cs;
        }
    }

    if (!out)
        out = fopen("omtest.out", "w");
    if (!out)
        return 0;

    LARGE_INTEGER perfStart, perfEnd, perfFreq;

    QueryPerformanceCounter(&perfStart);

    for (i = 0; i < cThreads; i++)
    {
        hThread[i] = CreateThread(NULL, 0, CreateParserThread, &a[i], 0, &idThread[i]);
    }
    WaitForMultipleObjects(cThreads, hThread, TRUE, INFINITE);

    QueryPerformanceCounter(&perfEnd);
    QueryPerformanceFrequency(&perfFreq);

    __int64 i64Time = *((__int64 *) &perfEnd) - *((__int64 *) &perfStart);
    __int64 i64perf = *((__int64 *) &perfFreq);
    i64Time = i64Time * 1000 / i64perf;
//    fprintf(stderr, "Total time is %d milliseconds\n", (int) i64Time);
//    fprintf(stderr, "Time per thread is %d\n", i64Time/cThreads);

    for (i = 0; i < cThreads; i++)
        CloseHandle(hThread[i]);

    DeleteCriticalSection(&cs);
    fclose(out);
    return 0;
}


DWORD WINAPI
CreateParserThread(void * pv)
{
    Args   * p = (Args *) pv;

    HRESULT hr = CoInitialize(NULL);

    ParseXML(p->pchFile, p->iCount, p->bQuiet, p->bPause);

    CoUninitialize();

    return 0;
}