/*
 * XML test
 *
 * Copyright 2008 Piotr Caban
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS
#define CONST_VTABLE

#include <stdio.h>
#include "windows.h"
#include "ole2.h"
#include "msxml2.h"
#include "ocidl.h"

#include "wine/test.h"

typedef enum _CH {
    CH_ENDTEST,
    CH_PUTDOCUMENTLOCATOR,
    CH_STARTDOCUMENT,
    CH_ENDDOCUMENT,
    CH_STARTPREFIXMAPPING,
    CH_ENDPREFIXMAPPING,
    CH_STARTELEMENT,
    CH_ENDELEMENT,
    CH_CHARACTERS,
    CH_IGNORABLEWHITESPACE,
    CH_PROCESSINGINSTRUCTION,
    CH_SKIPPEDENTITY
} CH;

static const WCHAR szSimpleXML[] = {
'<','?','x','m','l',' ','v','e','r','s','i','o','n','=','\"','1','.','0','\"',' ','?','>','\n',
'<','B','a','n','k','A','c','c','o','u','n','t','>','\n',
' ',' ',' ','<','N','u','m','b','e','r','>','1','2','3','4','<','/','N','u','m','b','e','r','>','\n',
' ',' ',' ','<','N','a','m','e','>','C','a','p','t','a','i','n',' ','A','h','a','b','<','/','N','a','m','e','>','\n',
'<','/','B','a','n','k','A','c','c','o','u','n','t','>','\n','\0'
};

static const WCHAR szCarriageRetTest[] = {
'<','?','x','m','l',' ','v','e','r','s','i','o','n','=','"','1','.','0','"','?','>','\r','\n',
'<','B','a','n','k','A','c','c','o','u','n','t','>','\r','\n',
'\t','<','N','u','m','b','e','r','>','1','2','3','4','<','/','N','u','m','b','e','r','>','\r','\n',
'\t','<','N','a','m','e','>','C','a','p','t','a','i','n',' ','A','h','a','b','<','/','N','a','m','e','>','\r','\n',
'<','/','B','a','n','k','A','c','c','o','u','n','t','>','\0'
};

static const CHAR szTestXML[] =
"<?xml version=\"1.0\" ?>\n"
"<BankAccount>\n"
"   <Number>1234</Number>\n"
"   <Name>Captain Ahab</Name>\n"
"</BankAccount>\n";

typedef struct _contenthandlercheck {
    CH id;
    int line;
    int column;
    const char *arg1;
    const char *arg2;
    const char *arg3;
} content_handler_test;

static content_handler_test contentHandlerTest1[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0 },
    { CH_STARTDOCUMENT, 0, 0 },
    { CH_STARTELEMENT, 2, 14, "", "BankAccount", "BankAccount" },
    { CH_CHARACTERS, 2, 14, "\n   " },
    { CH_STARTELEMENT, 3, 12, "", "Number", "Number" },
    { CH_CHARACTERS, 3, 12, "1234" },
    { CH_ENDELEMENT, 3, 18, "", "Number", "Number" },
    { CH_CHARACTERS, 3, 25, "\n   " },
    { CH_STARTELEMENT, 4, 10, "", "Name", "Name" },
    { CH_CHARACTERS, 4, 10, "Captain Ahab" },
    { CH_ENDELEMENT, 4, 24, "", "Name", "Name" },
    { CH_CHARACTERS, 4, 29, "\n" },
    { CH_ENDELEMENT, 5, 3, "", "BankAccount", "BankAccount" },
    { CH_ENDDOCUMENT, 0, 0 },
    { CH_ENDTEST }
};

static content_handler_test contentHandlerTest2[] = {
    { CH_PUTDOCUMENTLOCATOR, 0, 0 },
    { CH_STARTDOCUMENT, 0, 0 },
    { CH_STARTELEMENT, 2, 14, "", "BankAccount", "BankAccount" },
    { CH_CHARACTERS, 2, 14, "\n" },
    { CH_CHARACTERS, 2, 16, "\t" },
    { CH_STARTELEMENT, 3, 10, "", "Number", "Number" },
    { CH_CHARACTERS, 3, 10, "1234" },
    { CH_ENDELEMENT, 3, 16, "", "Number", "Number" },
    { CH_CHARACTERS, 3, 23, "\n" },
    { CH_CHARACTERS, 3, 25, "\t" },
    { CH_STARTELEMENT, 4, 8, "", "Name", "Name" },
    { CH_CHARACTERS, 4, 8, "Captain Ahab" },
    { CH_ENDELEMENT, 4, 22, "", "Name", "Name" },
    { CH_CHARACTERS, 4, 27, "\n" },
    { CH_ENDELEMENT, 5, 3, "", "BankAccount", "BankAccount" },
    { CH_ENDDOCUMENT, 0, 0 },
    { CH_ENDTEST }
};

static content_handler_test *expectCall;
static ISAXLocator *locator;

static const char *debugstr_wn(const WCHAR *szStr, int len)
{
    static char buf[1024];
    WideCharToMultiByte(CP_ACP, 0, szStr, len, buf, sizeof(buf), NULL, NULL);
    return buf;
}

static void test_saxstr(unsigned line, const WCHAR *szStr, int nStr, const char *szTest)
{
    WCHAR buf[1024];
    int len;

    if(!szTest) {
        ok_(__FILE__,line) (szStr == NULL, "szStr != NULL\n");
        ok_(__FILE__,line) (nStr == 0, "nStr = %d, expected 0\n", nStr);
        return;
    }

    len = strlen(szTest);
    ok_(__FILE__,line) (len == nStr, "nStr = %d, expected %d (%s)\n", nStr, len, szTest);
    if(len != nStr)
        return;

    MultiByteToWideChar(CP_ACP, 0, szTest, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok_(__FILE__,line) (!memcmp(szStr, buf, len*sizeof(WCHAR)), "unexpected szStr %s, expected %s\n",
                        debugstr_wn(szStr, nStr), szTest);
}

static BOOL test_expect_call(CH id)
{
    ok(expectCall->id == id, "unexpected call %d, expected %d\n", id, expectCall->id);
    return expectCall->id == id;
}

static void test_locator(unsigned line, int loc_line, int loc_column)
{
    int rcolumn, rline;
    ISAXLocator_getLineNumber(locator, &rline);
    ISAXLocator_getColumnNumber(locator, &rcolumn);

    ok_(__FILE__,line) (rline == loc_line,
            "unexpected line %d, expected %d\n", rline, loc_line);
    ok_(__FILE__,line) (rcolumn == loc_column,
            "unexpected column %d, expected %d\n", rcolumn, loc_column);
}

static HRESULT WINAPI contentHandler_QueryInterface(
        ISAXContentHandler* iface,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ISAXContentHandler))
    {
        *ppvObject = iface;
    }
    else
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

static ULONG WINAPI contentHandler_AddRef(
        ISAXContentHandler* iface)
{
    return 2;
}

static ULONG WINAPI contentHandler_Release(
        ISAXContentHandler* iface)
{
    return 1;
}

static HRESULT WINAPI contentHandler_putDocumentLocator(
        ISAXContentHandler* iface,
        ISAXLocator *pLocator)
{
    if(!test_expect_call(CH_PUTDOCUMENTLOCATOR))
        return E_FAIL;

    locator = pLocator;
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_startDocument(
        ISAXContentHandler* iface)
{
    if(!test_expect_call(CH_STARTDOCUMENT))
        return E_FAIL;

    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_endDocument(
        ISAXContentHandler* iface)
{
    if(!test_expect_call(CH_ENDDOCUMENT))
        return E_FAIL;

    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_startPrefixMapping(
        ISAXContentHandler* iface,
        const WCHAR *pPrefix,
        int nPrefix,
        const WCHAR *pUri,
        int nUri)
{
    if(!test_expect_call(CH_ENDDOCUMENT))
        return E_FAIL;

    test_saxstr(__LINE__, pPrefix, nPrefix, expectCall->arg1);
    test_saxstr(__LINE__, pUri, nUri, expectCall->arg2);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_endPrefixMapping(
        ISAXContentHandler* iface,
        const WCHAR *pPrefix,
        int nPrefix)
{
    if(!test_expect_call(CH_ENDPREFIXMAPPING))
        return E_FAIL;

    test_saxstr(__LINE__, pPrefix, nPrefix, expectCall->arg1);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_startElement(
        ISAXContentHandler* iface,
        const WCHAR *pNamespaceUri,
        int nNamespaceUri,
        const WCHAR *pLocalName,
        int nLocalName,
        const WCHAR *pQName,
        int nQName,
        ISAXAttributes *pAttr)
{
    if(!test_expect_call(CH_STARTELEMENT))
        return E_FAIL;

    test_saxstr(__LINE__, pNamespaceUri, nNamespaceUri, expectCall->arg1);
    test_saxstr(__LINE__, pLocalName, nLocalName, expectCall->arg2);
    test_saxstr(__LINE__, pQName, nQName, expectCall->arg3);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_endElement(
        ISAXContentHandler* iface,
        const WCHAR *pNamespaceUri,
        int nNamespaceUri,
        const WCHAR *pLocalName,
        int nLocalName,
        const WCHAR *pQName,
        int nQName)
{
    if(!test_expect_call(CH_ENDELEMENT))
        return E_FAIL;

    test_saxstr(__LINE__, pNamespaceUri, nNamespaceUri, expectCall->arg1);
    test_saxstr(__LINE__, pLocalName, nLocalName, expectCall->arg2);
    test_saxstr(__LINE__, pQName, nQName, expectCall->arg3);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_characters(
        ISAXContentHandler* iface,
        const WCHAR *pChars,
        int nChars)
{
    if(!test_expect_call(CH_CHARACTERS))
        return E_FAIL;

    test_saxstr(__LINE__, pChars, nChars, expectCall->arg1);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_ignorableWhitespace(
        ISAXContentHandler* iface,
        const WCHAR *pChars,
        int nChars)
{
    if(!test_expect_call(CH_IGNORABLEWHITESPACE))
        return E_FAIL;

    test_saxstr(__LINE__, pChars, nChars, expectCall->arg1);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_processingInstruction(
        ISAXContentHandler* iface,
        const WCHAR *pTarget,
        int nTarget,
        const WCHAR *pData,
        int nData)
{
    if(!test_expect_call(CH_PROCESSINGINSTRUCTION))
        return E_FAIL;

    test_saxstr(__LINE__, pTarget, nTarget, expectCall->arg1);
    test_saxstr(__LINE__, pData, nData, expectCall->arg2);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}

static HRESULT WINAPI contentHandler_skippedEntity(
        ISAXContentHandler* iface,
        const WCHAR *pName,
        int nName)
{
    if(!test_expect_call(CH_SKIPPEDENTITY))
        return E_FAIL;

    test_saxstr(__LINE__, pName, nName, expectCall->arg1);
    test_locator(__LINE__, expectCall->line, expectCall->column);

    expectCall++;
    return S_OK;
}


static const ISAXContentHandlerVtbl contentHandlerVtbl =
{
    contentHandler_QueryInterface,
    contentHandler_AddRef,
    contentHandler_Release,
    contentHandler_putDocumentLocator,
    contentHandler_startDocument,
    contentHandler_endDocument,
    contentHandler_startPrefixMapping,
    contentHandler_endPrefixMapping,
    contentHandler_startElement,
    contentHandler_endElement,
    contentHandler_characters,
    contentHandler_ignorableWhitespace,
    contentHandler_processingInstruction,
    contentHandler_skippedEntity
};

static ISAXContentHandler contentHandler = { &contentHandlerVtbl };

static HRESULT WINAPI isaxerrorHandler_QueryInterface(
        ISAXErrorHandler* iface,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ISAXErrorHandler))
    {
        *ppvObject = iface;
    }
    else
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

static ULONG WINAPI isaxerrorHandler_AddRef(
        ISAXErrorHandler* iface)
{
    return 2;
}

static ULONG WINAPI isaxerrorHandler_Release(
        ISAXErrorHandler* iface)
{
    return 1;
}

static HRESULT WINAPI isaxerrorHandler_error(
        ISAXErrorHandler* iface,
        ISAXLocator *pLocator,
        const WCHAR *pErrorMessage,
        HRESULT hrErrorCode)
{
    return S_OK;
}

static HRESULT WINAPI isaxerrorHandler_fatalError(
        ISAXErrorHandler* iface,
        ISAXLocator *pLocator,
        const WCHAR *pErrorMessage,
        HRESULT hrErrorCode)
{
    return S_OK;
}

static HRESULT WINAPI isaxerrorHanddler_ignorableWarning(
        ISAXErrorHandler* iface,
        ISAXLocator *pLocator,
        const WCHAR *pErrorMessage,
        HRESULT hrErrorCode)
{
    return S_OK;
}

static const ISAXErrorHandlerVtbl errorHandlerVtbl =
{
    isaxerrorHandler_QueryInterface,
    isaxerrorHandler_AddRef,
    isaxerrorHandler_Release,
    isaxerrorHandler_error,
    isaxerrorHandler_fatalError,
    isaxerrorHanddler_ignorableWarning
};

static ISAXErrorHandler errorHandler = { &errorHandlerVtbl };

static void test_saxreader(void)
{
    HRESULT hr;
    ISAXXMLReader *reader = NULL;
    VARIANT var;
    ISAXContentHandler *lpContentHandler;
    ISAXErrorHandler *lpErrorHandler;
    SAFEARRAY *pSA;
    SAFEARRAYBOUND SADim[1];
    char *pSAData = NULL;
    IStream *iStream;
    ULARGE_INTEGER liSize;
    LARGE_INTEGER liPos;
    ULONG bytesWritten;
    HANDLE file;
    static const CHAR testXmlA[] = "test.xml";
    static const WCHAR testXmlW[] = {'t','e','s','t','.','x','m','l',0};
    IXMLDOMDocument *domDocument;
    BSTR bstrData;
    VARIANT_BOOL vBool;

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_ISAXXMLReader, (LPVOID*)&reader);

    if(FAILED(hr))
    {
        skip("Failed to create SAXXMLReader instance\n");
        return;
    }

    hr = ISAXXMLReader_getContentHandler(reader, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER, got %08x\n", hr);

    hr = ISAXXMLReader_getErrorHandler(reader, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER, got %08x\n", hr);

    hr = ISAXXMLReader_getContentHandler(reader, &lpContentHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(lpContentHandler == NULL, "Expected %p, got %p\n", NULL, lpContentHandler);

    hr = ISAXXMLReader_getErrorHandler(reader, &lpErrorHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(lpErrorHandler == NULL, "Expected %p, got %p\n", NULL, lpErrorHandler);

    hr = ISAXXMLReader_putContentHandler(reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = ISAXXMLReader_putContentHandler(reader, &contentHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = ISAXXMLReader_putErrorHandler(reader, &errorHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = ISAXXMLReader_getContentHandler(reader, &lpContentHandler);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(lpContentHandler == &contentHandler, "Expected %p, got %p\n", &contentHandler, lpContentHandler);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(szSimpleXML);

    expectCall = contentHandlerTest1;
    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);

    SADim[0].lLbound= 0;
    SADim[0].cElements= sizeof(szTestXML)-1;
    pSA = SafeArrayCreate(VT_UI1, 1, SADim);
    SafeArrayAccessData(pSA, (void**)&pSAData);
    memcpy(pSAData, szTestXML, sizeof(szTestXML)-1);
    SafeArrayUnaccessData(pSA);
    V_VT(&var) = VT_ARRAY|VT_UI1;
    V_ARRAY(&var) = pSA;

    expectCall = contentHandlerTest1;
    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);

    SafeArrayDestroy(pSA);

    CreateStreamOnHGlobal(NULL, TRUE, &iStream);
    liSize.QuadPart = strlen(szTestXML);
    IStream_SetSize(iStream, liSize);
    IStream_Write(iStream, (void const*)szTestXML, strlen(szTestXML), &bytesWritten);
    liPos.QuadPart = 0;
    IStream_Seek(iStream, liPos, STREAM_SEEK_SET, NULL);
    V_VT(&var) = VT_UNKNOWN|VT_DISPATCH;
    V_UNKNOWN(&var) = (IUnknown*)iStream;

    expectCall = contentHandlerTest1;
    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);

    IStream_Release(iStream);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(szCarriageRetTest);

    expectCall = contentHandlerTest2;
    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);

    file = CreateFileA(testXmlA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Could not create file: %u\n", GetLastError());
    WriteFile(file, szTestXML, sizeof(szTestXML)-1, &bytesWritten, NULL);
    CloseHandle(file);

    expectCall = contentHandlerTest1;
    hr = ISAXXMLReader_parseURL(reader, testXmlW);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);

    DeleteFileA(testXmlA);

    hr = CoCreateInstance(&CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXMLDOMDocument, (LPVOID*)&domDocument);
    if(FAILED(hr))
    {
        skip("Failed to create DOMDocument instance\n");
        return;
    }
    bstrData = SysAllocString(szSimpleXML);
    hr = IXMLDOMDocument_loadXML(domDocument, bstrData, &vBool);
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown*)domDocument;

    expectCall = contentHandlerTest2;
    hr = ISAXXMLReader_parse(reader, var);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    test_expect_call(CH_ENDTEST);
    IXMLDOMDocument_Release(domDocument);

    ISAXXMLReader_Release(reader);
}

START_TEST(saxreader)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "failed to init com\n");

    test_saxreader();

    CoUninitialize();
}
