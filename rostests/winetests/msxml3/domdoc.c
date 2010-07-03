/*
 * XML test
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2007-2008 Alistair Leslie-Hughes
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

#include "windows.h"
#include "ole2.h"
#include "objsafe.h"
#include "xmldom.h"
#include "msxml2.h"
#include "msxml2did.h"
#include "dispex.h"
#include <stdio.h>
#include <assert.h>

#include "wine/test.h"

#include "initguid.h"

DEFINE_GUID(IID_IObjectSafety, 0xcb5bdc81, 0x93c1, 0x11cf, 0x8f,0x20, 0x00,0x80,0x5f,0x2c,0xd0,0x64);

static const WCHAR szEmpty[] = { 0 };
static const WCHAR szIncomplete[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',0
};
static const WCHAR szComplete1[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',
    '<','o','p','e','n','>','<','/','o','p','e','n','>','\n',0
};
static const WCHAR szComplete2[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',
    '<','o','>','<','/','o','>','\n',0
};
static const WCHAR szComplete3[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',
    '<','a','>','<','/','a','>','\n',0
};
static const WCHAR szComplete4[] = {
    '<','?','x','m','l',' ','v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',
    '<','l','c',' ','d','l','=','\'','s','t','r','1','\'','>','\n',
        '<','b','s',' ','v','r','=','\'','s','t','r','2','\'',' ','s','z','=','\'','1','2','3','4','\'','>',
            'f','n','1','.','t','x','t','\n',
        '<','/','b','s','>','\n',
        '<','p','r',' ','i','d','=','\'','s','t','r','3','\'',' ','v','r','=','\'','1','.','2','.','3','\'',' ',
                    'p','n','=','\'','w','i','n','e',' ','2','0','0','5','0','8','0','4','\'','>','\n',
            'f','n','2','.','t','x','t','\n',
        '<','/','p','r','>','\n',
        '<','e','m','p','t','y','>','<','/','e','m','p','t','y','>','\n',
        '<','f','o','>','\n',
            '<','b','a','>','\n',
                'f','1','\n',
            '<','/','b','a','>','\n',
        '<','/','f','o','>','\n',
    '<','/','l','c','>','\n',0
};
static const WCHAR szComplete5[] = {
    '<','S',':','s','e','a','r','c','h',' ','x','m','l','n','s',':','D','=','"','D','A','V',':','"',' ',
    'x','m','l','n','s',':','C','=','"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','o','f','f','i','c','e',':','c','l','i','p','g','a','l','l','e','r','y','"',
    ' ','x','m','l','n','s',':','S','=','"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','o','f','f','i','c','e',':','c','l','i','p','g','a','l','l','e','r','y',':','s','e','a','r','c','h','"','>',
        '<','S',':','s','c','o','p','e','>',
            '<','S',':','d','e','e','p','>','/','<','/','S',':','d','e','e','p','>',
        '<','/','S',':','s','c','o','p','e','>',
        '<','S',':','c','o','n','t','e','n','t','f','r','e','e','t','e','x','t','>',
            '<','C',':','t','e','x','t','o','r','p','r','o','p','e','r','t','y','/','>',
            'c','o','m','p','u','t','e','r',
        '<','/','S',':','c','o','n','t','e','n','t','f','r','e','e','t','e','x','t','>',
    '<','/','S',':','s','e','a','r','c','h','>',0
};

static const WCHAR szComplete6[] = {
    '<','?','x','m','l',' ','v','e','r','s','i','o','n','=','\'','1','.','0','\'',' ',
    'e','n','c','o','d','i','n','g','=','\'','W','i','n','d','o','w','s','-','1','2','5','2','\'','?','>','\n',
    '<','o','p','e','n','>','<','/','o','p','e','n','>','\n',0
};

static const CHAR szNonUnicodeXML[] =
"<?xml version='1.0' encoding='Windows-1252'?>\n"
"<open></open>\n";

static const CHAR szExampleXML[] =
"<?xml version='1.0' encoding='utf-8'?>\n"
"<root xmlns:foo='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'>\n"
"    <elem>\n"
"        <a>A1 field</a>\n"
"        <b>B1 field</b>\n"
"        <c>C1 field</c>\n"
"        <description xmlns:foo='http://www.winehq.org' xmlns:bar='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'>\n"
"            <html xmlns='http://www.w3.org/1999/xhtml'>\n"
"                This is <strong>a</strong> <i>description</i>. <bar:x/>\n"
"            </html>\n"
"        </description>\n"
"    </elem>\n"
"\n"
"    <elem>\n"
"        <a>A2 field</a>\n"
"        <b>B2 field</b>\n"
"        <c type=\"old\">C2 field</c>\n"
"    </elem>\n"
"\n"
"    <elem xmlns='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'>\n"
"        <a>A3 field</a>\n"
"        <b>B3 field</b>\n"
"        <c>C3 field</c>\n"
"    </elem>\n"
"\n"
"    <elem>\n"
"        <a>A4 field</a>\n"
"        <b>B4 field</b>\n"
"        <foo:c>C4 field</foo:c>\n"
"    </elem>\n"
"</root>\n";

static  const CHAR szTransformXML[] =
"<?xml version=\"1.0\"?>\n"
"<greeting>\n"
"Hello World\n"
"</greeting>";

static  const CHAR szTransformSSXML[] =
"<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">\n"
"   <xsl:output method=\"html\"/>\n"
"   <xsl:template match=\"/\">\n"
"       <xsl:apply-templates select=\"greeting\"/>\n"
"   </xsl:template>\n"
"   <xsl:template match=\"greeting\">\n"
"       <html>\n"
"           <body>\n"
"               <h1>\n"
"                   <xsl:value-of select=\".\"/>\n"
"               </h1>\n"
"           </body>\n"
"       </html>\n"
"   </xsl:template>\n"
"</xsl:stylesheet>";

static  const CHAR szTransformOutput[] =
"<html><body><h1>"
"Hello World"
"</h1></body></html>";

static const CHAR szTypeValueXML[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
"<root xmlns:dt=\"urn:schemas-microsoft-com:datatypes\">\n"
"   <string>Wine</string>\n"
"   <string2 dt:dt=\"string\">String</string2>\n"
"   <number dt:dt=\"number\">12.44</number>\n"
"   <number2 dt:dt=\"NUMbEr\">-3.71e3</number2>\n"
"   <int dt:dt=\"int\">-13</int>\n"
"   <fixed dt:dt=\"fixed.14.4\">7322.9371</fixed>\n"
"   <bool dt:dt=\"boolean\">1</bool>\n"
"   <datetime dt:dt=\"datetime\">2009-11-18T03:21:33.12</datetime>\n"
"   <datetimetz dt:dt=\"datetime.tz\">2003-07-11T11:13:57+03:00</datetimetz>\n"
"   <date dt:dt=\"date\">3721-11-01</date>\n"
"   <time dt:dt=\"time\">13:57:12.31321</time>\n"
"   <timetz dt:dt=\"time.tz\">23:21:01.13+03:21</timetz>\n"
"   <i1 dt:dt=\"i1\">-13</i1>\n"
"   <i2 dt:dt=\"i2\">31915</i2>\n"
"   <i4 dt:dt=\"i4\">-312232</i4>\n"
"   <ui1 dt:dt=\"ui1\">123</ui1>\n"
"   <ui2 dt:dt=\"ui2\">48282</ui2>\n"
"   <ui4 dt:dt=\"ui4\">949281</ui4>\n"
"   <r4 dt:dt=\"r4\">213124.0</r4>\n"
"   <r8 dt:dt=\"r8\">0.412</r8>\n"
"   <float dt:dt=\"float\">41221.421</float>\n"
"   <uuid dt:dt=\"uuid\">333C7BC4-460F-11D0-BC04-0080C7055a83</uuid>\n"
"   <binhex dt:dt=\"bin.hex\">fffca012003c</binhex>\n"
"   <binbase64 dt:dt=\"bin.base64\">YmFzZTY0IHRlc3Q=</binbase64>\n"
"</root>";

static const CHAR szBasicTransformSSXMLPart1[] =
"<?xml version=\"1.0\"?>"
"<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" >"
"<xsl:output method=\"html\"/>\n"
"<xsl:template match=\"/\">"
"<HTML><BODY><TABLE>"
"        <xsl:apply-templates select='document(\"";

static const CHAR szBasicTransformSSXMLPart2[] =
"\")/bottle/wine'>"
"           <xsl:sort select=\"cost\"/><xsl:sort select=\"name\"/>"
"        </xsl:apply-templates>"
"</TABLE></BODY></HTML>"
"</xsl:template>"
"<xsl:template match=\"bottle\">"
"   <TR><xsl:apply-templates select=\"name\" /><xsl:apply-templates select=\"cost\" /></TR>"
"</xsl:template>"
"<xsl:template match=\"name\">"
"   <TD><xsl:apply-templates /></TD>"
"</xsl:template>"
"<xsl:template match=\"cost\">"
"   <TD><xsl:apply-templates /></TD>"
"</xsl:template>"
"</xsl:stylesheet>";

static const CHAR szBasicTransformXML[] =
"<?xml version=\"1.0\"?><bottle><wine><name>Wine</name><cost>$25.00</cost></wine></bottle>";

static const CHAR szBasicTransformOutput[] =
"<HTML><BODY><TABLE><TD>Wine</TD><TD>$25.00</TD></TABLE></BODY></HTML>";

static const WCHAR szNonExistentFile[] = {
    'c', ':', '\\', 'N', 'o', 'n', 'e', 'x', 'i', 's', 't', 'e', 'n', 't', '.', 'x', 'm', 'l', 0
};
static const WCHAR szNonExistentAttribute[] = {
    'n','o','n','E','x','i','s','i','t','i','n','g','A','t','t','r','i','b','u','t','e',0
};
static const WCHAR szDocument[] = {
    '#', 'd', 'o', 'c', 'u', 'm', 'e', 'n', 't', 0
};

static const WCHAR szOpen[] = { 'o','p','e','n',0 };
static WCHAR szdl[] = { 'd','l',0 };
static const WCHAR szvr[] = { 'v','r',0 };
static const WCHAR szlc[] = { 'l','c',0 };
static WCHAR szbs[] = { 'b','s',0 };
static const WCHAR szstr1[] = { 's','t','r','1',0 };
static const WCHAR szstr2[] = { 's','t','r','2',0 };
static const WCHAR szstar[] = { '*',0 };
static const WCHAR szfn1_txt[] = {'f','n','1','.','t','x','t',0};

static WCHAR szComment[] = {'A',' ','C','o','m','m','e','n','t',0 };
static WCHAR szCommentXML[] = {'<','!','-','-','A',' ','C','o','m','m','e','n','t','-','-','>',0 };
static WCHAR szCommentNodeText[] = {'#','c','o','m','m','e','n','t',0 };

static WCHAR szElement[] = {'E','l','e','T','e','s','t', 0 };
static WCHAR szElementXML[]  = {'<','E','l','e','T','e','s','t','/','>',0 };
static WCHAR szElementXML2[] = {'<','E','l','e','T','e','s','t',' ','A','t','t','r','=','"','"','/','>',0 };
static WCHAR szElementXML3[] = {'<','E','l','e','T','e','s','t',' ','A','t','t','r','=','"','"','>',
                                'T','e','s','t','i','n','g','N','o','d','e','<','/','E','l','e','T','e','s','t','>',0 };
static WCHAR szElementXML4[] = {'<','E','l','e','T','e','s','t',' ','A','t','t','r','=','"','"','>',
                                '&','a','m','p',';','x',' ',0x2103,'<','/','E','l','e','T','e','s','t','>',0 };

static WCHAR szAttribute[] = {'A','t','t','r',0 };
static WCHAR szAttributeXML[] = {'A','t','t','r','=','"','"',0 };

static WCHAR szCData[] = {'[','1',']','*','2','=','3',';',' ','&','g','e','e',' ','t','h','a','t','s',
                          ' ','n','o','t',' ','r','i','g','h','t','!', 0};
static WCHAR szCDataXML[] = {'<','!','[','C','D','A','T','A','[','[','1',']','*','2','=','3',';',' ','&',
                             'g','e','e',' ','t','h','a','t','s',' ','n','o','t',' ','r','i','g','h','t',
                             '!',']',']','>',0};
static WCHAR szCDataNodeText[] = {'#','c','d','a','t','a','-','s','e','c','t','i','o','n',0 };
static WCHAR szDocFragmentText[] = {'#','d','o','c','u','m','e','n','t','-','f','r','a','g','m','e','n','t',0 };

static WCHAR szEntityRef[] = {'e','n','t','i','t','y','r','e','f',0 };
static WCHAR szEntityRefXML[] = {'&','e','n','t','i','t','y','r','e','f',';',0 };
static WCHAR szStrangeChars[] = {'&','x',' ',0x2103, 0};

#define expect_bstr_eq_and_free(bstr, expect) { \
    BSTR bstrExp = alloc_str_from_narrow(expect); \
    ok(lstrcmpW(bstr, bstrExp) == 0, "String differs\n"); \
    SysFreeString(bstr); \
    SysFreeString(bstrExp); \
}

#define expect_eq(expr, value, type, format) { type ret = (expr); ok((value) == ret, #expr " expected " format " got " format "\n", value, ret); }

#define ole_check(expr) { \
    HRESULT r = expr; \
    ok(r == S_OK, #expr " returned %x\n", r); \
}

#define ole_expect(expr, expect) { \
    HRESULT r = expr; \
    ok(r == (expect), #expr " returned %x, expected %x\n", r, expect); \
}

#define double_eq(x, y) ok((x)-(y)<=1e-14*(x) && (x)-(y)>=-1e-14*(x), "expected %.16g, got %.16g\n", x, y)

static BSTR alloc_str_from_narrow(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    BSTR ret = SysAllocStringLen(NULL, len - 1);  /* NUL character added automatically */
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

BSTR alloced_bstrs[256];
int alloced_bstrs_count = 0;

static BSTR _bstr_(const char *str)
{
    assert(alloced_bstrs_count < sizeof(alloced_bstrs)/sizeof(alloced_bstrs[0]));
    alloced_bstrs[alloced_bstrs_count] = alloc_str_from_narrow(str);
    return alloced_bstrs[alloced_bstrs_count++];
}

static void free_bstrs(void)
{
    int i;
    for (i = 0; i < alloced_bstrs_count; i++)
        SysFreeString(alloced_bstrs[i]);
    alloced_bstrs_count = 0;
}

static VARIANT _variantbstr_(const char *str)
{
    VARIANT v;
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = _bstr_(str);
    return v;
}

static BOOL compareIgnoreReturns(BSTR sLeft, BSTR sRight)
{
    for (;;)
    {
        while (*sLeft == '\r' || *sLeft == '\n') sLeft++;
        while (*sRight == '\r' || *sRight == '\n') sRight++;
        if (*sLeft != *sRight) return FALSE;
        if (!*sLeft) return TRUE;
        sLeft++;
        sRight++;
    }
}

static BOOL compareIgnoreReturnsWhitespace(BSTR sLeft, BSTR sRight)
{
    /* MSXML3 inserts whitespace where as libxml doesn't. */
    for (;;)
    {
        while (*sLeft == '\r' || *sLeft == '\n' || *sLeft == ' ') sLeft++;
        while (*sRight == '\r' || *sRight == '\n' || *sRight == ' ') sRight++;
        if (*sLeft != *sRight) return FALSE;
        if (!*sLeft) return TRUE;
        sLeft++;
        sRight++;
    }
}

static void get_str_for_type(DOMNodeType type, char *buf)
{
    switch (type)
    {
        case NODE_ATTRIBUTE:
            strcpy(buf, "A");
            break;
        case NODE_ELEMENT:
            strcpy(buf, "E");
            break;
        case NODE_DOCUMENT:
            strcpy(buf, "D");
            break;
        default:
            wsprintfA(buf, "[%d]", type);
    }
}

#define test_disp(u) _test_disp(__LINE__,u)
static void _test_disp(unsigned line, IUnknown *unk)
{
    DISPID dispid = DISPID_XMLDOM_NODELIST_RESET;
    IDispatchEx *dispex;
    DWORD dwProps = 0;
    BSTR sName;
    UINT ticnt;
    IUnknown *pUnk;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IDispatchEx, (void**)&dispex);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IDispatch: %08x\n", hres);
    if(FAILED(hres))
        return;

    ticnt = 0xdeadbeef;
    hres = IDispatchEx_GetTypeInfoCount(dispex, &ticnt);
    ok_(__FILE__,line) (hres == S_OK, "GetTypeInfoCount failed: %08x\n", hres);
    ok_(__FILE__,line) (ticnt == 1, "ticnt=%u\n", ticnt);

    sName = SysAllocString( szstar );
    hres = IDispatchEx_DeleteMemberByName(dispex, sName, fdexNameCaseSensitive);
    ok(hres == E_NOTIMPL, "expected E_NOTIMPL got %08x\n", hres);
    SysFreeString( sName );

    hres = IDispatchEx_DeleteMemberByDispID(dispex, dispid);
    ok(hres == E_NOTIMPL, "expected E_NOTIMPL got %08x\n", hres);

    hres = IDispatchEx_GetMemberProperties(dispex, dispid, grfdexPropCanAll, &dwProps);
    ok(hres == E_NOTIMPL, "expected E_NOTIMPL got %08x\n", hres);
    ok(dwProps == 0, "expected 0 got %d\n", dwProps);

    hres = IDispatchEx_GetMemberName(dispex, dispid, &sName);
    ok(hres == E_NOTIMPL, "expected E_NOTIMPL got %08x\n", hres);
    if(SUCCEEDED(hres))
        SysFreeString(sName);

    hres = IDispatchEx_GetNextDispID(dispex, fdexEnumDefault, DISPID_XMLDOM_NODELIST_RESET, &dispid);
    ok(hres == E_NOTIMPL, "expected E_NOTIMPL got %08x\n", hres);

    hres = IDispatchEx_GetNameSpaceParent(dispex, &pUnk);
    ok(hres == E_NOTIMPL, "expected E_NOTIMPL got %08x\n", hres);
    if(hres == S_OK && pUnk)
        IUnknown_Release(pUnk);

    IDispatchEx_Release(dispex);
}

static int get_node_position(IXMLDOMNode *node)
{
    HRESULT r;
    int pos = 0;

    IXMLDOMNode_AddRef(node);
    do
    {
        IXMLDOMNode *new_node;

        pos++;
        r = IXMLDOMNode_get_previousSibling(node, &new_node);
        ok(SUCCEEDED(r), "get_previousSibling failed\n");
        IXMLDOMNode_Release(node);
        node = new_node;
    } while (r == S_OK);
    return pos;
}

static void node_to_string(IXMLDOMNode *node, char *buf)
{
    HRESULT r = S_OK;
    DOMNodeType type;

    if (node == NULL)
    {
        lstrcpyA(buf, "(null)");
        return;
    }

    IXMLDOMNode_AddRef(node);
    while (r == S_OK)
    {
        IXMLDOMNode *new_node;

        ole_check(IXMLDOMNode_get_nodeType(node, &type));
        get_str_for_type(type, buf);
        buf+=strlen(buf);

        if (type == NODE_ATTRIBUTE)
        {
            BSTR bstr;
            ole_check(IXMLDOMNode_get_nodeName(node, &bstr));
            *(buf++) = '\'';
            wsprintfA(buf, "%ws", bstr);
            buf += strlen(buf);
            *(buf++) = '\'';
            SysFreeString(bstr);

            r = IXMLDOMNode_selectSingleNode(node, _bstr_(".."), &new_node);
        }
        else
        {
            r = IXMLDOMNode_get_parentNode(node, &new_node);
            wsprintf(buf, "%d", get_node_position(node));
            buf += strlen(buf);
        }

        ok(SUCCEEDED(r), "get_parentNode failed (%08x)\n", r);
        IXMLDOMNode_Release(node);
        node = new_node;
        if (r == S_OK)
            *(buf++) = '.';
    }

    *buf = 0;
}

static char *list_to_string(IXMLDOMNodeList *list)
{
    static char buf[4096];
    char *pos = buf;
    LONG len = 0;
    int i;

    if (list == NULL)
    {
        lstrcpyA(buf, "(null)");
        return buf;
    }
    ole_check(IXMLDOMNodeList_get_length(list, &len));
    for (i = 0; i < len; i++)
    {
        IXMLDOMNode *node;
        if (i > 0)
            *(pos++) = ' ';
        ole_check(IXMLDOMNodeList_nextNode(list, &node));
        node_to_string(node, pos);
        pos += strlen(pos);
        IXMLDOMNode_Release(node);
    }
    *pos = 0;
    return buf;
}

#define expect_node(node, expstr) { char str[4096]; node_to_string(node, str); ok(strcmp(str, expstr)==0, "Invalid node: %s, expected %s\n", str, expstr); }
#define expect_list_and_release(list, expstr) { char *str = list_to_string(list); ok(strcmp(str, expstr)==0, "Invalid node list: %s, expected %s\n", str, expstr); if (list) IXMLDOMNodeList_Release(list); }

static void test_domdoc( void )
{
    HRESULT r;
    IXMLDOMDocument *doc = NULL;
    IXMLDOMParseError *error;
    IXMLDOMElement *element = NULL;
    IXMLDOMNode *node;
    IXMLDOMText *nodetext = NULL;
    IXMLDOMComment *node_comment = NULL;
    IXMLDOMAttribute *node_attr = NULL;
    IXMLDOMNode *nodeChild = NULL;
    IXMLDOMProcessingInstruction *nodePI = NULL;
    ISupportErrorInfo *support_error = NULL;
    VARIANT_BOOL b;
    VARIANT var;
    BSTR str;
    LONG code;
    LONG nLength = 0;
    WCHAR buff[100];

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    test_disp((IUnknown*)doc);

if (0)
{
    /* crashes on native */
    r = IXMLDOMDocument_loadXML( doc, (BSTR)0x1, NULL );
}

    /* try some stupid things */
    r = IXMLDOMDocument_loadXML( doc, NULL, NULL );
    ok( r == S_FALSE, "loadXML succeeded\n");

    b = VARIANT_TRUE;
    r = IXMLDOMDocument_loadXML( doc, NULL, &b );
    ok( r == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "failed to load XML string\n");

    /* try to load a document from a nonexistent file */
    b = VARIANT_TRUE;
    str = SysAllocString( szNonExistentFile );
    VariantInit(&var);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = str;

    r = IXMLDOMDocument_load( doc, var, &b);
    ok( r == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    /* try load an empty document */
    b = VARIANT_TRUE;
    str = SysAllocString( szEmpty );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_get_async( doc, &b );
    ok( r == S_OK, "get_async failed (%08x)\n", r);
    ok( b == VARIANT_TRUE, "Wrong default value\n");

    /* check that there's no document element */
    element = NULL;
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_FALSE, "should be no document element\n");

    /* try finding a node */
    node = NULL;
    str = SysAllocString( szstr1 );
    r = IXMLDOMDocument_selectSingleNode( doc, str, &node );
    ok( r == S_FALSE, "ret %08x\n", r );
    SysFreeString( str );

    b = VARIANT_TRUE;
    str = SysAllocString( szIncomplete );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    /* check that there's no document element */
    element = (IXMLDOMElement*)1;
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_FALSE, "should be no document element\n");
    ok( element == NULL, "Element should be NULL\n");

    /* test for BSTR handling, pass broken BSTR */
    memcpy(&buff[2], szComplete1, sizeof(szComplete1));
    /* just a big length */
    *(DWORD*)buff = 0xf0f0;
    b = VARIANT_FALSE;
    r = IXMLDOMDocument_loadXML( doc, &buff[2], &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    /* loadXML ignores the encoding attribute and always expects Unicode */
    b = VARIANT_FALSE;
    str = SysAllocString( szComplete6 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    /* try a BSTR containing a Windows-1252 document */
    b = VARIANT_TRUE;
    str = SysAllocStringByteLen( szNonUnicodeXML, sizeof(szNonUnicodeXML) - 1 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    /* try to load something valid */
    b = VARIANT_FALSE;
    str = SysAllocString( szComplete1 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    /* check if nodename is correct */
    r = IXMLDOMDocument_get_nodeName( doc, NULL );
    ok ( r == E_INVALIDARG, "get_nodeName (NULL) wrong code\n");

    str = (BSTR)0xdeadbeef;
    r = IXMLDOMDocument_get_baseName( doc, &str );
    ok ( r == S_FALSE, "got 0x%08x\n", r);
    ok (str == NULL, "got %p\n", str);

    /* content doesn't matter here */
    str = NULL;
    r = IXMLDOMDocument_get_nodeName( doc, &str );
    ok ( r == S_OK, "get_nodeName wrong code\n");
    ok ( str != NULL, "str is null\n");
    ok( !lstrcmpW( str, szDocument ), "incorrect nodeName\n");
    SysFreeString( str );

    /* test put_text */
    r = IXMLDOMDocument_put_text( doc, _bstr_("Should Fail") );
    ok( r == E_FAIL, "ret %08x\n", r );

    /* check that there's a document element */
    element = NULL;
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "should be a document element\n");
    if( element )
    {
        IObjectIdentity *ident;
        BSTR tag = NULL;

        test_disp((IUnknown*)element);

        r = IXMLDOMElement_QueryInterface( element, &IID_IObjectIdentity, (LPVOID*)&ident );
        ok( r == E_NOINTERFACE, "ret %08x\n", r);

        r = IXMLDOMElement_get_tagName( element, NULL );
        ok( r == E_INVALIDARG, "ret %08x\n", r);

        /* check if the tag is correct */
        r = IXMLDOMElement_get_tagName( element, &tag );
        ok( r == S_OK, "couldn't get tag name\n");
        ok( tag != NULL, "tag was null\n");
        ok( !lstrcmpW( tag, szOpen ), "incorrect tag name\n");
        SysFreeString( tag );

        /* figure out what happens if we try to reload the document */
        str = SysAllocString( szComplete2 );
        r = IXMLDOMDocument_loadXML( doc, str, &b );
        ok( r == S_OK, "loadXML failed\n");
        ok( b == VARIANT_TRUE, "failed to load XML string\n");
        SysFreeString( str );

        /* check if the tag is still correct */
        tag = NULL;
        r = IXMLDOMElement_get_tagName( element, &tag );
        ok( r == S_OK, "couldn't get tag name\n");
        ok( tag != NULL, "tag was null\n");
        ok( !lstrcmpW( tag, szOpen ), "incorrect tag name\n");
        SysFreeString( tag );

        IXMLDOMElement_Release( element );
        element = NULL;
    }

    /* as soon as we call loadXML again, the document element will disappear */
    b = 2;
    r = IXMLDOMDocument_loadXML( doc, NULL, NULL );
    ok( r == S_FALSE, "loadXML failed\n");
    ok( b == 2, "variant modified\n");
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_FALSE, "should be no document element\n");

    /* try to load something else simple and valid */
    b = VARIANT_FALSE;
    str = SysAllocString( szComplete3 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    /* try something a little more complicated */
    b = FALSE;
    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_get_parseError( doc, &error );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMParseError_get_errorCode( error, &code );
    ok( r == S_FALSE, "returns %08x\n", r );
    ok( code == 0, "code %d\n", code );
    IXMLDOMParseError_Release( error );

    /* test createTextNode */
    r = IXMLDOMDocument_createTextNode(doc, _bstr_(""), &nodetext);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMText_Release(nodetext);

    str = SysAllocString( szOpen );
    r = IXMLDOMDocument_createTextNode(doc, str, NULL);
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    r = IXMLDOMDocument_createTextNode(doc, str, &nodetext);
    ok( r == S_OK, "returns %08x\n", r );
    SysFreeString( str );
    if(nodetext)
    {
        IXMLDOMNamedNodeMap *pAttribs;

        r = IXMLDOMText_QueryInterface(nodetext, &IID_IXMLDOMElement, (LPVOID*)&element);
        ok(r == E_NOINTERFACE, "ret %08x\n", r );

        /* Text Last Child Checks */
        r = IXMLDOMText_get_lastChild(nodetext, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        nodeChild = (IXMLDOMNode*)0x1;
        r = IXMLDOMText_get_lastChild(nodetext, &nodeChild);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok(nodeChild == NULL, "nodeChild not NULL\n");

        /* test get_attributes */
        r = IXMLDOMText_get_attributes( nodetext, NULL );
        ok( r == E_INVALIDARG, "get_attributes returned wrong code\n");

        pAttribs = (IXMLDOMNamedNodeMap*)0x1;
        r = IXMLDOMText_get_attributes( nodetext, &pAttribs);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok( pAttribs == NULL, "pAttribs not NULL\n");

        /* test get_dataType */
        r = IXMLDOMText_get_dataType(nodetext, &var);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok( V_VT(&var) == VT_NULL, "incorrect dataType type\n");
        VariantClear(&var);

        /* test length property */
        r = IXMLDOMText_get_length(nodetext, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );
        ok(nLength == 4, "expected 4 got %d\n", nLength);

        /* test nodeTypeString */
        r = IXMLDOMText_get_nodeTypeString(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("text") ), "incorrect nodeTypeString string\n");
        SysFreeString(str);

        /* put data Tests */
        r = IXMLDOMText_put_data(nodetext, _bstr_("This &is a ; test <>\\"));
        ok(r == S_OK, "ret %08x\n", r );

        /* get data Tests */
        r = IXMLDOMText_get_data(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect put_data string\n");
        SysFreeString(str);

        /* Confirm XML text is good */
        r = IXMLDOMText_get_xml(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This &amp;is a ; test &lt;&gt;\\") ), "incorrect xml string\n");
        SysFreeString(str);

        /* Confirm we get the put_data Text back */
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect xml string\n");
        SysFreeString(str);

        /* test substringData */
        r = IXMLDOMText_substringData(nodetext, 0, 4, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        /* test substringData - Invalid offset */
        str = (BSTR)&szElement;
        r = IXMLDOMText_substringData(nodetext, -1, 4, &str);
        ok(r == E_INVALIDARG, "ret %08x\n", r );
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Invalid offset */
        str = (BSTR)&szElement;
        r = IXMLDOMText_substringData(nodetext, 30, 0, &str);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Invalid size */
        str = (BSTR)&szElement;
        r = IXMLDOMText_substringData(nodetext, 0, -1, &str);
        ok(r == E_INVALIDARG, "ret %08x\n", r );
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Invalid size */
        str = (BSTR)&szElement;
        r = IXMLDOMText_substringData(nodetext, 2, 0, &str);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Start of string */
        r = IXMLDOMText_substringData(nodetext, 0, 4, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This") ), "incorrect substringData string\n");
        SysFreeString(str);

        /* test substringData - Middle of string */
        r = IXMLDOMText_substringData(nodetext, 13, 4, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("test") ), "incorrect substringData string\n");
        SysFreeString(str);

        /* test substringData - End of string */
        r = IXMLDOMText_substringData(nodetext, 20, 4, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("\\") ), "incorrect substringData string\n");
        SysFreeString(str);

        /* test appendData */
        r = IXMLDOMText_appendData(nodetext, NULL);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_appendData(nodetext, _bstr_(""));
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_appendData(nodetext, _bstr_("Append"));
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\Append") ), "incorrect get_text string\n");
        SysFreeString(str);

        /* test insertData */
        str = SysAllocStringLen(NULL, 0);
        r = IXMLDOMText_insertData(nodetext, -1, str);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, -1, NULL);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 1000, str);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 1000, NULL);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 0, NULL);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 0, str);
        ok(r == S_OK, "ret %08x\n", r );
        SysFreeString(str);

        r = IXMLDOMText_insertData(nodetext, -1, _bstr_("Inserting"));
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 1000, _bstr_("Inserting"));
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 0, _bstr_("Begin "));
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 17, _bstr_("Middle"));
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 39, _bstr_(" End"));
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("Begin This &is a Middle; test <>\\Append End") ), "incorrect get_text string\n");
        SysFreeString(str);

        /* delete data */
        /* invalid arguments */
        r = IXMLDOMText_deleteData(nodetext, -1, 1);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        r = IXMLDOMText_deleteData(nodetext, 0, 0);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_deleteData(nodetext, 0, -1);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );
        ok(nLength == 43, "expected 43 got %d\n", nLength);

        r = IXMLDOMText_deleteData(nodetext, nLength, 1);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_deleteData(nodetext, nLength+1, 1);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        /* delete from start */
        r = IXMLDOMText_deleteData(nodetext, 0, 5);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );
        ok(nLength == 38, "expected 38 got %d\n", nLength);

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        /* whitespace preserving needs to be handled here */
        todo_wine ok( !lstrcmpW( str, _bstr_("This &is a Middle; test <>\\Append End") ), "incorrect get_text string\n");
        SysFreeString(str);

        /* delete from end */
        r = IXMLDOMText_deleteData(nodetext, 35, 3);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );
        ok(nLength == 35, "expected 35 got %d\n", nLength);

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        todo_wine ok( !lstrcmpW( str, _bstr_("This &is a Middle; test <>\\Append") ), "incorrect get_text string\n");
        SysFreeString(str);

        /* delete from inside */
        r = IXMLDOMText_deleteData(nodetext, 1, 33);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );
        ok(nLength == 2, "expected 2 got %d\n", nLength);

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        todo_wine ok( !lstrcmpW( str, _bstr_("") ), "incorrect get_text string\n");
        SysFreeString(str);

        /* delete whole data ... */
        r = IXMLDOMText_get_length(nodetext, &nLength);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_deleteData(nodetext, 0, nLength);
        ok(r == S_OK, "ret %08x\n", r );
        /* ... and try again with empty string */
        r = IXMLDOMText_deleteData(nodetext, 0, nLength);
        ok(r == S_OK, "ret %08x\n", r );

        /* test put_data */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(szstr1);
        r = IXMLDOMText_put_nodeValue(nodetext, var);
        ok(r == S_OK, "ret %08x\n", r );
        VariantClear(&var);

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, szstr1 ), "incorrect get_text string\n");
        SysFreeString(str);

        /* test put_data */
        V_VT(&var) = VT_I4;
        V_I4(&var) = 99;
        r = IXMLDOMText_put_nodeValue(nodetext, var);
        ok(r == S_OK, "ret %08x\n", r );
        VariantClear(&var);

        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("99") ), "incorrect get_text string\n");
        SysFreeString(str);

        /* ::replaceData() */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(szstr1);
        r = IXMLDOMText_put_nodeValue(nodetext, var);
        ok(r == S_OK, "ret %08x\n", r );
        VariantClear(&var);

        r = IXMLDOMText_replaceData(nodetext, 6, 0, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string\n");
        SysFreeString(str);

        r = IXMLDOMText_replaceData(nodetext, 0, 0, NULL);
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string\n");
        SysFreeString(str);

        /* NULL pointer means delete */
        r = IXMLDOMText_replaceData(nodetext, 0, 1, NULL);
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("tr1") ), "incorrect get_text string\n");
        SysFreeString(str);

        /* empty string means delete */
        r = IXMLDOMText_replaceData(nodetext, 0, 1, _bstr_(""));
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("r1") ), "incorrect get_text string\n");
        SysFreeString(str);

        /* zero count means insert */
        r = IXMLDOMText_replaceData(nodetext, 0, 0, _bstr_("a"));
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("ar1") ), "incorrect get_text string\n");
        SysFreeString(str);

        r = IXMLDOMText_replaceData(nodetext, 0, 2, NULL);
        ok(r == S_OK, "ret %08x\n", r );

        r = IXMLDOMText_insertData(nodetext, 0, _bstr_("m"));
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("m1") ), "incorrect get_text string\n");
        SysFreeString(str);

        /* nonempty string, count greater than its length */
        r = IXMLDOMText_replaceData(nodetext, 0, 2, _bstr_("a1.2"));
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("a1.2") ), "incorrect get_text string\n");
        SysFreeString(str);

        /* nonempty string, count less than its length */
        r = IXMLDOMText_replaceData(nodetext, 0, 1, _bstr_("wine"));
        ok(r == S_OK, "ret %08x\n", r );
        r = IXMLDOMText_get_text(nodetext, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("wine1.2") ), "incorrect get_text string\n");
        SysFreeString(str);

        IXMLDOMText_Release( nodetext );
    }

    /* test Create Comment */
    r = IXMLDOMDocument_createComment(doc, NULL, NULL);
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    node_comment = (IXMLDOMComment*)0x1;

    /* empty comment */
    r = IXMLDOMDocument_createComment(doc, _bstr_(""), &node_comment);
    ok( r == S_OK, "returns %08x\n", r );
    str = (BSTR)0x1;
    r = IXMLDOMComment_get_data(node_comment, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty string data\n");
    IXMLDOMComment_Release(node_comment);
    SysFreeString(str);

    r = IXMLDOMDocument_createComment(doc, NULL, &node_comment);
    ok( r == S_OK, "returns %08x\n", r );
    str = (BSTR)0x1;
    r = IXMLDOMComment_get_data(node_comment, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && (SysStringLen(str) == 0), "expected empty string data\n");
    IXMLDOMComment_Release(node_comment);
    SysFreeString(str);

    str = SysAllocString(szComment);
    r = IXMLDOMDocument_createComment(doc, str, &node_comment);
    SysFreeString(str);
    ok( r == S_OK, "returns %08x\n", r );
    if(node_comment)
    {
        /* Last Child Checks */
        r = IXMLDOMComment_get_lastChild(node_comment, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        nodeChild = (IXMLDOMNode*)0x1;
        r = IXMLDOMComment_get_lastChild(node_comment, &nodeChild);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok(nodeChild == NULL, "pLastChild not NULL\n");

        /* baseName */
        str = (BSTR)0xdeadbeef;
        IXMLDOMComment_get_baseName(node_comment, &str);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok(str == NULL, "Expected NULL\n");

        IXMLDOMComment_Release( node_comment );
    }

    /* test Create Attribute */
    str = SysAllocString(szAttribute);
    r = IXMLDOMDocument_createAttribute(doc, NULL, NULL);
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    r = IXMLDOMDocument_createAttribute(doc, str, &node_attr);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMText_Release( node_attr);
    SysFreeString(str);

    /* test Processing Instruction */
    str = SysAllocStringLen(NULL, 0);
    r = IXMLDOMDocument_createProcessingInstruction(doc, str, str, NULL);
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    r = IXMLDOMDocument_createProcessingInstruction(doc, NULL, str, &nodePI);
    ok( r == E_FAIL, "returns %08x\n", r );
    r = IXMLDOMDocument_createProcessingInstruction(doc, str, str, &nodePI);
    ok( r == E_FAIL, "returns %08x\n", r );
    SysFreeString(str);

    r = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"), _bstr_("version=\"1.0\""), &nodePI);
    ok( r == S_OK, "returns %08x\n", r );
    if(nodePI)
    {
        /* Last Child Checks */
        r = IXMLDOMProcessingInstruction_get_lastChild(nodePI, NULL);
        ok(r == E_INVALIDARG, "ret %08x\n", r );

        nodeChild = (IXMLDOMNode*)0x1;
        r = IXMLDOMProcessingInstruction_get_lastChild(nodePI, &nodeChild);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok(nodeChild == NULL, "nodeChild not NULL\n");

        r = IXMLDOMProcessingInstruction_get_dataType(nodePI, &var);
        ok(r == S_FALSE, "ret %08x\n", r );
        ok( V_VT(&var) == VT_NULL, "incorrect dataType type\n");
        VariantClear(&var);

        /* test nodeName */
        r = IXMLDOMProcessingInstruction_get_nodeName(nodePI, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("xml") ), "incorrect nodeName string\n");
        SysFreeString(str);

        /* test baseName */
        str = (BSTR)0x1;
        r = IXMLDOMProcessingInstruction_get_baseName(nodePI, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("xml") ), "incorrect nodeName string\n");
        SysFreeString(str);

        /* test Target */
        r = IXMLDOMProcessingInstruction_get_target(nodePI, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("xml") ), "incorrect target string\n");
        SysFreeString(str);

        /* test nodeTypeString */
        r = IXMLDOMProcessingInstruction_get_nodeTypeString(nodePI, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("processinginstruction") ), "incorrect nodeTypeString string\n");
        SysFreeString(str);

        /* test get_nodeValue */
        r = IXMLDOMProcessingInstruction_get_nodeValue(nodePI, &var);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( V_BSTR(&var), _bstr_("version=\"1.0\"") ), "incorrect data string\n");
        VariantClear(&var);

        /* test get_data */
        r = IXMLDOMProcessingInstruction_get_data(nodePI, &str);
        ok(r == S_OK, "ret %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("version=\"1.0\"") ), "incorrect data string\n");
        SysFreeString(str);

        /* test put_data */
        r = IXMLDOMProcessingInstruction_put_data(nodePI, _bstr_("version=\"1.0\" encoding=\"UTF-8\""));
        ok(r == E_FAIL, "ret %08x\n", r );

        /* test put_data */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(szOpen);  /* Doesn't matter what the string is, cannot set an xml node. */
        r = IXMLDOMProcessingInstruction_put_nodeValue(nodePI, var);
        ok(r == E_FAIL, "ret %08x\n", r );
        VariantClear(&var);

        /* test get nodeName */
        r = IXMLDOMProcessingInstruction_get_nodeName(nodePI, &str);
        ok( !lstrcmpW( str, _bstr_("xml") ), "incorrect nodeName string\n");
        ok(r == S_OK, "ret %08x\n", r );
        SysFreeString(str);

        IXMLDOMProcessingInstruction_Release(nodePI);
    }

    r = IXMLDOMDocument_QueryInterface( doc, &IID_ISupportErrorInfo, (LPVOID*)&support_error );
    ok( r == S_OK, "ret %08x\n", r );
    if(r == S_OK)
    {
        r = ISupportErrorInfo_InterfaceSupportsErrorInfo( support_error, &IID_IXMLDOMDocument );
        todo_wine ok( r == S_OK, "ret %08x\n", r );
        ISupportErrorInfo_Release( support_error );
    }

    r = IXMLDOMDocument_Release( doc );
    ok( r == 0, "document ref count incorrect\n");

    free_bstrs();
}

static void test_persiststreaminit(void)
{
    IXMLDOMDocument *doc;
    IPersistStreamInit *streaminit;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void**)&doc );
    ok( hr == S_OK, "failed with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&streaminit);
    ok( hr == S_OK, "failed with 0x%08x\n", hr );

    hr = IPersistStreamInit_InitNew(streaminit);
    ok( hr == S_OK, "failed with 0x%08x\n", hr );

    IXMLDOMDocument_Release(doc);
}

static void test_domnode( void )
{
    HRESULT r;
    IXMLDOMDocument *doc = NULL, *owner = NULL;
    IXMLDOMElement *element = NULL;
    IXMLDOMNamedNodeMap *map = NULL;
    IXMLDOMNode *node = NULL, *next = NULL;
    IXMLDOMNodeList *list = NULL;
    IXMLDOMAttribute *attr = NULL;
    DOMNodeType type = NODE_INVALID;
    VARIANT_BOOL b;
    BSTR str;
    VARIANT var;
    LONG count;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    b = FALSE;
    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    b = 1;
    r = IXMLDOMNode_hasChildNodes( doc, &b );
    ok( r == S_OK, "hasChildNoes bad return\n");
    ok( b == VARIANT_TRUE, "hasChildNoes wrong result\n");

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "should be a document element\n");
    ok( element != NULL, "should be an element\n");

    VariantInit(&var);
    ok( V_VT(&var) == VT_EMPTY, "variant init failed\n");

    r = IXMLDOMNode_get_nodeValue( doc, NULL );
    ok(r == E_INVALIDARG, "get_nodeValue ret %08x\n", r );

    r = IXMLDOMNode_get_nodeValue( doc, &var );
    ok( r == S_FALSE, "nextNode returned wrong code\n");
    ok( V_VT(&var) == VT_NULL, "variant wasn't empty\n");
    ok( V_BSTR(&var) == NULL, "variant value wasn't null\n");

    if (element)
    {
        owner = NULL;
        r = IXMLDOMNode_get_ownerDocument( element, &owner );
        ok( r == S_OK, "get_ownerDocument return code\n");
        ok( owner != doc, "get_ownerDocument return\n");
        IXMLDOMDocument_Release(owner);

        type = NODE_INVALID;
        r = IXMLDOMNode_get_nodeType( element, &type);
        ok( r == S_OK, "getNamedItem returned wrong code\n");
        ok( type == NODE_ELEMENT, "node not an element\n");

        str = NULL;
        r = IXMLDOMNode_get_baseName( element, &str );
        ok( r == S_OK, "get_baseName returned wrong code\n");
        ok( lstrcmpW(str,szlc) == 0, "basename was wrong\n");
        SysFreeString(str);

        /* check if nodename is correct */
        r = IXMLDOMElement_get_nodeName( element, NULL );
        ok ( r == E_INVALIDARG, "get_nodeName (NULL) wrong code\n");
    
        /* content doesn't matter here */
        str = NULL;
        r = IXMLDOMElement_get_nodeName( element, &str );
        ok ( r == S_OK, "get_nodeName wrong code\n");
        ok ( str != NULL, "str is null\n");
        ok( !lstrcmpW( str, szlc ), "incorrect nodeName\n");
        SysFreeString( str );

        str = SysAllocString( szNonExistentFile );	
        V_VT(&var) = VT_I4;
        V_I4(&var) = 0x1234;
        r = IXMLDOMElement_getAttribute( element, str, &var );
        ok( r == E_FAIL, "getAttribute ret %08x\n", r );
        ok( V_VT(&var) == VT_NULL || V_VT(&var) == VT_EMPTY, "vt = %x\n", V_VT(&var));
        VariantClear(&var);

        r = IXMLDOMElement_getAttributeNode( element, str, NULL);
        ok( r == E_FAIL, "getAttributeNode ret %08x\n", r );

        attr = (IXMLDOMAttribute*)0xdeadbeef;
        r = IXMLDOMElement_getAttributeNode( element, str, &attr);
        ok( r == E_FAIL, "getAttributeNode ret %08x\n", r );
        ok( attr == NULL, "getAttributeNode ret %p, expected NULL\n", attr );
        SysFreeString( str );

        attr = (IXMLDOMAttribute*)0xdeadbeef;
        str = SysAllocString( szNonExistentAttribute );
        r = IXMLDOMElement_getAttributeNode( element, str, &attr);
        ok( r == S_FALSE, "getAttributeNode ret %08x\n", r );
        ok( attr == NULL, "getAttributeNode ret %p, expected NULL\n", attr );
        SysFreeString( str );

        str = SysAllocString( szdl );	
        V_VT(&var) = VT_I4;
        V_I4(&var) = 0x1234;
        r = IXMLDOMElement_getAttribute( element, str, &var );
        ok( r == S_OK, "getAttribute ret %08x\n", r );
        ok( V_VT(&var) == VT_BSTR, "vt = %x\n", V_VT(&var));
        ok( !lstrcmpW(V_BSTR(&var), szstr1), "wrong attr value\n");
        VariantClear( &var );

        r = IXMLDOMElement_getAttribute( element, NULL, &var );
        ok( r == E_INVALIDARG, "getAttribute ret %08x\n", r );

        r = IXMLDOMElement_getAttribute( element, str, NULL );
        ok( r == E_INVALIDARG, "getAttribute ret %08x\n", r );

        attr = NULL;
        r = IXMLDOMElement_getAttributeNode( element, str, &attr);
        ok( r == S_OK, "GetAttributeNode ret %08x\n", r );
        ok( attr != NULL, "getAttributeNode returned NULL\n" );
        if (attr)
        {
            r = IXMLDOMAttribute_get_parentNode( attr, NULL );
            ok( r == E_INVALIDARG, "Expected E_INVALIDARG, ret %08x\n", r );

            /* attribute doesn't have a parent in msxml interpretation */
            node = (IXMLDOMNode*)0xdeadbeef;
            r = IXMLDOMAttribute_get_parentNode( attr, &node );
            ok( r == S_FALSE, "Expected S_FALSE, ret %08x\n", r );
            ok( node == NULL, "Expected NULL, got %p\n", node );

            IXMLDOMAttribute_Release(attr);
        }

        SysFreeString( str );

        r = IXMLDOMElement_get_attributes( element, &map );
        ok( r == S_OK, "get_attributes returned wrong code\n");
        ok( map != NULL, "should be attributes\n");

        b = 1;
        r = IXMLDOMNode_hasChildNodes( element, &b );
        ok( r == S_OK, "hasChildNoes bad return\n");
        ok( b == VARIANT_TRUE, "hasChildNoes wrong result\n");
    }
    else
        ok( FALSE, "no element\n");

    if (map)
    {
        ISupportErrorInfo *support_error;
        r = IXMLDOMNamedNodeMap_QueryInterface( map, &IID_ISupportErrorInfo, (LPVOID*)&support_error );
        ok( r == S_OK, "ret %08x\n", r );

        r = ISupportErrorInfo_InterfaceSupportsErrorInfo( support_error, &IID_IXMLDOMNamedNodeMap );
todo_wine
{
        ok( r == S_OK, "ret %08x\n", r );
}
        ISupportErrorInfo_Release( support_error );

        str = SysAllocString( szdl );
        r = IXMLDOMNamedNodeMap_getNamedItem( map, str, &node );
        ok( r == S_OK, "getNamedItem returned wrong code\n");
        ok( node != NULL, "should be attributes\n");
        IXMLDOMNode_Release(node);
        SysFreeString( str );

        str = SysAllocString( szdl );
        r = IXMLDOMNamedNodeMap_getNamedItem( map, str, NULL );
        ok( r == E_INVALIDARG, "getNamedItem should return E_INVALIDARG\n");
        SysFreeString( str );

        /* something that isn't in szComplete4 */
        str = SysAllocString( szOpen );
        node = (IXMLDOMNode *) 1;
        r = IXMLDOMNamedNodeMap_getNamedItem( map, str, &node );
        ok( r == S_FALSE, "getNamedItem found a node that wasn't there\n");
        ok( node == NULL, "getNamedItem should have returned NULL\n");
        SysFreeString( str );

	/* test indexed access of attributes */
        r = IXMLDOMNamedNodeMap_get_length( map, NULL );
        ok ( r == E_INVALIDARG, "get_length should return E_INVALIDARG\n");

        r = IXMLDOMNamedNodeMap_get_length( map, &count );
        ok ( r == S_OK, "get_length wrong code\n");
        ok ( count == 1, "get_length != 1\n");

        node = NULL;
        r = IXMLDOMNamedNodeMap_get_item( map, -1, &node);
        ok ( r == S_FALSE, "get_item (-1) wrong code\n");
        ok ( node == NULL, "there is no node\n");

        node = NULL;
        r = IXMLDOMNamedNodeMap_get_item( map, 1, &node);
        ok ( r == S_FALSE, "get_item (1) wrong code\n");
        ok ( node == NULL, "there is no attribute\n");

        node = NULL;
        r = IXMLDOMNamedNodeMap_get_item( map, 0, &node);
        ok ( r == S_OK, "get_item (0) wrong code\n");
        ok ( node != NULL, "should be attribute\n");

        r = IXMLDOMNode_get_nodeName( node, NULL );
        ok ( r == E_INVALIDARG, "get_nodeName (NULL) wrong code\n");

        /* content doesn't matter here */
        str = NULL;
        r = IXMLDOMNode_get_nodeName( node, &str );
        ok ( r == S_OK, "get_nodeName wrong code\n");
        ok ( str != NULL, "str is null\n");
        ok( !lstrcmpW( str, szdl ), "incorrect node name\n");
        SysFreeString( str );

        /* test sequential access of attributes */
        node = NULL;
        r = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( r == S_OK, "nextNode (first time) wrong code\n");
        ok ( node != NULL, "nextNode, should be attribute\n");

        r = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( r != S_OK, "nextNode (second time) wrong code\n");
        ok ( node == NULL, "nextNode, there is no attribute\n");

        r = IXMLDOMNamedNodeMap_reset( map );
        ok ( r == S_OK, "reset should return S_OK\n");

        r = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( r == S_OK, "nextNode (third time) wrong code\n");
        ok ( node != NULL, "nextNode, should be attribute\n");
    }
    else
        ok( FALSE, "no map\n");

    if (node)
    {
        type = NODE_INVALID;
        r = IXMLDOMNode_get_nodeType( node, &type);
        ok( r == S_OK, "getNamedItem returned wrong code\n");
        ok( type == NODE_ATTRIBUTE, "node not an attribute\n");

        str = NULL;
        r = IXMLDOMNode_get_baseName( node, NULL );
        ok( r == E_INVALIDARG, "get_baseName returned wrong code\n");

        str = NULL;
        r = IXMLDOMNode_get_baseName( node, &str );
        ok( r == S_OK, "get_baseName returned wrong code\n");
        ok( lstrcmpW(str,szdl) == 0, "basename was wrong\n");
        SysFreeString( str );

        r = IXMLDOMNode_get_nodeValue( node, &var );
        ok( r == S_OK, "returns %08x\n", r );
        ok( V_VT(&var) == VT_BSTR, "vt %x\n", V_VT(&var));
        ok( !lstrcmpW(V_BSTR(&var), szstr1), "nodeValue incorrect\n");
        VariantClear(&var);

        r = IXMLDOMNode_get_childNodes( node, NULL );
        ok( r == E_INVALIDARG, "get_childNodes returned wrong code\n");

        r = IXMLDOMNode_get_childNodes( node, &list );
        ok( r == S_OK, "get_childNodes returned wrong code\n");

        if (list)
        {
            r = IXMLDOMNodeList_nextNode( list, &next );
            ok( r == S_OK, "nextNode returned wrong code\n");
        }
        else
            ok( FALSE, "no childlist\n");

        if (next)
        {
            b = 1;
            r = IXMLDOMNode_hasChildNodes( next, &b );
            ok( r == S_FALSE, "hasChildNoes bad return\n");
            ok( b == VARIANT_FALSE, "hasChildNoes wrong result\n");

            type = NODE_INVALID;
            r = IXMLDOMNode_get_nodeType( next, &type);
            ok( r == S_OK, "getNamedItem returned wrong code\n");
            ok( type == NODE_TEXT, "node not text\n");

            str = (BSTR) 1;
            r = IXMLDOMNode_get_baseName( next, &str );
            ok( r == S_FALSE, "get_baseName returned wrong code\n");
            ok( str == NULL, "basename was wrong\n");
            SysFreeString(str);
        }
        else
            ok( FALSE, "no next\n");

        if (next)
            IXMLDOMNode_Release( next );
        next = NULL;
        if (list)
            IXMLDOMNodeList_Release( list );
        list = NULL;
        if (node)
            IXMLDOMNode_Release( node );
    }
    else
        ok( FALSE, "no node\n");
    node = NULL;

    if (map)
        IXMLDOMNamedNodeMap_Release( map );

    /* now traverse the tree from the root element */
    if (element)
    {
        r = IXMLDOMNode_get_childNodes( element, &list );
        ok( r == S_OK, "get_childNodes returned wrong code\n");

        /* using get_item for child list doesn't advance the position */
        ole_check(IXMLDOMNodeList_get_item(list, 1, &node));
        expect_node(node, "E2.E2.D1");
        IXMLDOMNode_Release(node);
        ole_check(IXMLDOMNodeList_nextNode(list, &node));
        expect_node(node, "E1.E2.D1");
        IXMLDOMNode_Release(node);
        ole_check(IXMLDOMNodeList_reset(list));

        IXMLDOMNodeList_AddRef(list);
        expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E3.E2.D1 E4.E2.D1");
        ole_check(IXMLDOMNodeList_reset(list));

        node = (void*)0xdeadbeef;
        str = SysAllocString(szdl);
        r = IXMLDOMNode_selectSingleNode( element, str, &node );
        SysFreeString(str);
        ok( r == S_FALSE, "ret %08x\n", r );
        ok( node == NULL, "node %p\n", node );

        str = SysAllocString(szbs);
        r = IXMLDOMNode_selectSingleNode( element, str, &node );
        SysFreeString(str);
        ok( r == S_OK, "ret %08x\n", r );
        r = IXMLDOMNode_Release( node );
        ok( r == 0, "ret %08x\n", r );
    }
    else
        ok( FALSE, "no element\n");

    if (list)
    {
        r = IXMLDOMNodeList_QueryInterface(list, &IID_IDispatch, NULL);
        ok( r == E_INVALIDARG || r == E_POINTER, "ret %08x\n", r );

        r = IXMLDOMNodeList_get_item(list, 0, NULL);
        ok(r == E_INVALIDARG, "Exected E_INVALIDARG got %08x\n", r);

        r = IXMLDOMNodeList_get_length(list, NULL);
        ok(r == E_INVALIDARG, "Exected E_INVALIDARG got %08x\n", r);

        r = IXMLDOMNodeList_get_length( list, &count );
        ok( r == S_OK, "get_length returns %08x\n", r );
        ok( count == 4, "get_length got %d\n", count );

        r = IXMLDOMNodeList_nextNode(list, NULL);
        ok(r == E_INVALIDARG, "Exected E_INVALIDARG got %08x\n", r);

        r = IXMLDOMNodeList_nextNode( list, &node );
        ok( r == S_OK, "nextNode returned wrong code\n");
    }
    else
        ok( FALSE, "no list\n");

    if (node)
    {
        type = NODE_INVALID;
        r = IXMLDOMNode_get_nodeType( node, &type);
        ok( r == S_OK, "getNamedItem returned wrong code\n");
        ok( type == NODE_ELEMENT, "node not text\n");

        VariantInit(&var);
        ok( V_VT(&var) == VT_EMPTY, "variant init failed\n");
        r = IXMLDOMNode_get_nodeValue( node, &var );
        ok( r == S_FALSE, "nextNode returned wrong code\n");
        ok( V_VT(&var) == VT_NULL, "variant wasn't empty\n");
        ok( V_BSTR(&var) == NULL, "variant value wasn't null\n");

        r = IXMLDOMNode_hasChildNodes( node, NULL );
        ok( r == E_INVALIDARG, "hasChildNoes bad return\n");

        b = 1;
        r = IXMLDOMNode_hasChildNodes( node, &b );
        ok( r == S_OK, "hasChildNoes bad return\n");
        ok( b == VARIANT_TRUE, "hasChildNoes wrong result\n");

        str = NULL;
        r = IXMLDOMNode_get_baseName( node, &str );
        ok( r == S_OK, "get_baseName returned wrong code\n");
        ok( lstrcmpW(str,szbs) == 0, "basename was wrong\n");
        SysFreeString(str);
    }
    else
        ok( FALSE, "no node\n");

    if (node)
        IXMLDOMNode_Release( node );
    if (list)
        IXMLDOMNodeList_Release( list );
    if (element)
        IXMLDOMElement_Release( element );

    b = FALSE;
    str = SysAllocString( szComplete5 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    b = 1;
    r = IXMLDOMNode_hasChildNodes( doc, &b );
    ok( r == S_OK, "hasChildNoes bad return\n");
    ok( b == VARIANT_TRUE, "hasChildNoes wrong result\n");

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "should be a document element\n");
    ok( element != NULL, "should be an element\n");

    if (element)
    {
        static const WCHAR szSSearch[] = {'S',':','s','e','a','r','c','h',0};
        BSTR tag = NULL;

        /* check if the tag is correct */
        r = IXMLDOMElement_get_tagName( element, &tag );
        ok( r == S_OK, "couldn't get tag name\n");
        ok( tag != NULL, "tag was null\n");
        ok( !lstrcmpW( tag, szSSearch ), "incorrect tag name\n");
        SysFreeString( tag );
    }

    if (element)
        IXMLDOMElement_Release( element );
    ok(IXMLDOMDocument_Release( doc ) == 0, "document is not destroyed\n");
}

static void test_refs(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc = NULL;
    IXMLDOMElement *element = NULL;
    IXMLDOMNode *node = NULL, *node2;
    IXMLDOMNodeList *node_list = NULL;
    LONG ref;
    IUnknown *unk, *unk2;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;
    ref = IXMLDOMDocument_Release(doc);
    ok( ref == 0, "ref %d\n", ref);

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    ref = IXMLDOMDocument_AddRef( doc );
    ok( ref == 2, "ref %d\n", ref );
    ref = IXMLDOMDocument_AddRef( doc );
    ok( ref == 3, "ref %d\n", ref );
    IXMLDOMDocument_Release( doc );
    IXMLDOMDocument_Release( doc );

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "should be a document element\n");
    ok( element != NULL, "should be an element\n");

    ref = IXMLDOMDocument_AddRef( doc );
    ok( ref == 2, "ref %d\n", ref );
    IXMLDOMDocument_Release( doc );

    r = IXMLDOMElement_get_childNodes( element, &node_list );
    ok( r == S_OK, "rets %08x\n", r);

    ref = IXMLDOMNodeList_AddRef( node_list );
    ok( ref == 2, "ref %d\n", ref );
    IXMLDOMNodeList_Release( node_list );

    IXMLDOMNodeList_get_item( node_list, 0, &node );
    ok( r == S_OK, "rets %08x\n", r);

    IXMLDOMNodeList_get_item( node_list, 0, &node2 );
    ok( r == S_OK, "rets %08x\n", r);

    ref = IXMLDOMNode_AddRef( node );
    ok( ref == 2, "ref %d\n", ref );
    IXMLDOMNode_Release( node );

    ref = IXMLDOMNode_Release( node );
    ok( ref == 0, "ref %d\n", ref );
    ref = IXMLDOMNode_Release( node2 );
    ok( ref == 0, "ref %d\n", ref );

    ref = IXMLDOMNodeList_Release( node_list );
    ok( ref == 0, "ref %d\n", ref );

    ok( node != node2, "node %p node2 %p\n", node, node2 );

    ref = IXMLDOMDocument_Release( doc );
    ok( ref == 0, "ref %d\n", ref );

    ref = IXMLDOMElement_AddRef( element );
    todo_wine {
    ok( ref == 3, "ref %d\n", ref );
    }
    IXMLDOMElement_Release( element );

    /* IUnknown must be unique however we obtain it */
    r = IXMLDOMElement_QueryInterface( element, &IID_IUnknown, (LPVOID*)&unk );
    ok( r == S_OK, "rets %08x\n", r );
    r = IXMLDOMElement_QueryInterface( element, &IID_IXMLDOMNode, (LPVOID*)&node );
    ok( r == S_OK, "rets %08x\n", r );
    r = IXMLDOMNode_QueryInterface( node, &IID_IUnknown, (LPVOID*)&unk2 );
    ok( r == S_OK, "rets %08x\n", r );
    ok( unk == unk2, "unk %p unk2 %p\n", unk, unk2 );

    IUnknown_Release( unk2 );
    IUnknown_Release( unk );
    IXMLDOMNode_Release( node );

    IXMLDOMElement_Release( element );

}

static void test_create(void)
{
    static const WCHAR szOne[] = {'1',0};
    static const WCHAR szOneGarbage[] = {'1','G','a','r','b','a','g','e',0};
    HRESULT r;
    VARIANT var;
    BSTR str, name;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMComment *comment;
    IXMLDOMText *text;
    IXMLDOMCDATASection *cdata;
    IXMLDOMNode *root, *node, *child;
    IXMLDOMNamedNodeMap *attr_map;
    IUnknown *unk;
    LONG ref;
    LONG num;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    /* types not supported for creation */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_DOCUMENT;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_DOCUMENT_TYPE;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ENTITY;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_NOTATION;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    /* NODE_COMMENT */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_COMMENT;
    node = NULL;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node != NULL, "\n");

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)&comment);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMComment_get_data(comment, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMComment_Release(comment);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)&comment);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMComment_get_data(comment, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMComment_Release(comment);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_("blah"), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)&comment);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMComment_get_data(comment, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMComment_Release(comment);
    SysFreeString(str);

    /* NODE_TEXT */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_TEXT;
    node = NULL;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node != NULL, "\n");

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)&text);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMText_get_data(text, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMText_Release(text);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)&text);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMText_get_data(text, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMText_Release(text);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_("blah"), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)&text);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMText_get_data(text, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMText_Release(text);
    SysFreeString(str);

    /* NODE_CDATA_SECTION */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_CDATA_SECTION;
    node = NULL;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node != NULL, "\n");

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)&cdata);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMCDATASection_get_data(cdata, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMCDATASection_Release(cdata);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)&cdata);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMCDATASection_get_data(cdata, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMCDATASection_Release(cdata);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_("blah"), NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)&cdata);
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release(node);

    str = NULL;
    r = IXMLDOMCDATASection_get_data(cdata, &str);
    ok( r == S_OK, "returns %08x\n", r );
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMCDATASection_Release(cdata);
    SysFreeString(str);

    /* NODE_ATTRIBUTE */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    str = SysAllocString( szlc );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    if( SUCCEEDED(r) ) IXMLDOMNode_Release( node );
    SysFreeString(str);

    /* a name is required for attribute, try a BSTR with first null wchar */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    str = SysAllocString( szstr1 );
    str[0] = 0;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);
    SysFreeString(str);

    /* NODE_PROCESSING_INSTRUCTION */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_PROCESSING_INSTRUCTION;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_PROCESSING_INSTRUCTION;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_PROCESSING_INSTRUCTION;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_("pi"), NULL, NULL );
    ok( r == E_INVALIDARG, "returns %08x\n", r );

    /* NODE_ENTITY_REFERENCE */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ENTITY_REFERENCE;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ENTITY_REFERENCE;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    /* NODE_ELEMENT */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    node = (IXMLDOMNode*)0x1;
    r = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok( r == E_FAIL, "returns %08x\n", r );
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    str = SysAllocString( szlc );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    if( SUCCEEDED(r) ) IXMLDOMNode_Release( node );

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, NULL );
    ok( r == E_INVALIDARG, "returns %08x\n", r );

    V_VT(&var) = VT_R4;
    V_R4(&var) = NODE_ELEMENT;
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    if( SUCCEEDED(r) ) IXMLDOMNode_Release( node );

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szOne );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    if( SUCCEEDED(r) ) IXMLDOMNode_Release( node );
    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szOneGarbage );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == E_INVALIDARG, "returns %08x\n", r );
    if( SUCCEEDED(r) ) IXMLDOMNode_Release( node );
    VariantClear(&var);

    V_VT(&var) = VT_I4;
    V_I4(&var) = NODE_ELEMENT;
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMDocument_appendChild( doc, node, &root );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node == root, "%p %p\n", node, root );

    ref = IXMLDOMNode_AddRef( node );
    ok(ref == 3, "ref %d\n", ref);
    IXMLDOMNode_Release( node );

    ref = IXMLDOMNode_Release( node );
    ok(ref == 1, "ref %d\n", ref);
    SysFreeString( str );

    V_VT(&var) = VT_I4;
    V_I4(&var) = NODE_ELEMENT;
    str = SysAllocString( szbs );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    SysFreeString( str );

    ref = IXMLDOMNode_AddRef( node );
    ok(ref == 2, "ref = %d\n", ref);
    IXMLDOMNode_Release( node );

    r = IXMLDOMNode_QueryInterface( node, &IID_IUnknown, (LPVOID*)&unk );
    ok( r == S_OK, "returns %08x\n", r );

    ref = IXMLDOMNode_AddRef( unk );
    ok(ref == 3, "ref = %d\n", ref);
    IXMLDOMNode_Release( unk );

    V_VT(&var) = VT_EMPTY;
    r = IXMLDOMNode_insertBefore( root, (IXMLDOMNode*)unk, var, &child );
    ok( r == S_OK, "returns %08x\n", r );
    ok( unk == (IUnknown*)child, "%p %p\n", unk, child );
    IXMLDOMNode_Release( child );
    IUnknown_Release( unk );


    V_VT(&var) = VT_NULL;
    V_DISPATCH(&var) = (IDispatch*)node;
    r = IXMLDOMNode_insertBefore( root, node, var, &child );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node == child, "%p %p\n", node, child );
    IXMLDOMNode_Release( child );


    V_VT(&var) = VT_NULL;
    V_DISPATCH(&var) = (IDispatch*)node;
    r = IXMLDOMNode_insertBefore( root, node, var, NULL );
    ok( r == S_OK, "returns %08x\n", r );
    IXMLDOMNode_Release( node );

    r = IXMLDOMNode_QueryInterface( root, &IID_IXMLDOMElement, (LPVOID*)&element );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 0, "num %d\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szstr1 );
    name = SysAllocString( szdl );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 1, "num %d\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szstr2 );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 1, "num %d\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);
    r = IXMLDOMElement_getAttribute( element, name, &var );
    ok( r == S_OK, "returns %08x\n", r );
    ok( !lstrcmpW(V_BSTR(&var), szstr2), "wrong attr value\n");
    VariantClear(&var);
    SysFreeString(name);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szstr1 );
    name = SysAllocString( szlc );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 2, "num %d\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);
    SysFreeString(name);

    V_VT(&var) = VT_I4;
    V_I4(&var) = 10;
    name = SysAllocString( szbs );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    VariantClear(&var);
    r = IXMLDOMElement_getAttribute( element, name, &var );
    ok( r == S_OK, "returns %08x\n", r );
    ok( V_VT(&var) == VT_BSTR, "variant type %x\n", V_VT(&var));
    VariantClear(&var);
    SysFreeString(name);

    /* Create an Attribute */
    V_VT(&var) = VT_I4;
    V_I4(&var) = NODE_ATTRIBUTE;
    str = SysAllocString( szAttribute );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node != NULL, "node was null\n");
    SysFreeString(str);

    if(r == S_OK)
    {
        r = IXMLDOMNode_get_nodeTypeString(node, &str);
        ok( r == S_OK, "returns %08x\n", r );
        ok( !lstrcmpW( str, _bstr_("attribute") ), "incorrect nodeTypeString string\n");
        SysFreeString(str);
        IXMLDOMNode_Release( node );
    }

    IXMLDOMElement_Release( element );
    IXMLDOMNode_Release( root );
    IXMLDOMDocument_Release( doc );
}

static void test_getElementsByTagName(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMNodeList *node_list;
    LONG len;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    str = SysAllocString( szstar );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 6, "len %d\n", len );

    test_disp((IUnknown*)node_list);

    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    str = SysAllocString( szbs );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 1, "len %d\n", len );
    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    str = SysAllocString( szdl );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 0, "len %d\n", len );
    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    str = SysAllocString( szstr1 );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 0, "len %d\n", len );
    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    IXMLDOMDocument_Release( doc );
}

static void test_get_text(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMNode *node, *node2, *node3;
    IXMLDOMNode *nodeRoot;
    IXMLDOMNodeList *node_list;
    IXMLDOMNamedNodeMap *node_map;
    LONG len;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    str = SysAllocString( szbs );
    r = IXMLDOMDocument_getElementsByTagName( doc, str, &node_list );
    ok( r == S_OK, "ret %08x\n", r );
    SysFreeString(str);

    /* Test to get all child node text. */
    r = IXMLDOMDocument_QueryInterface(doc, &IID_IXMLDOMNode, (LPVOID*)&nodeRoot);
    ok( r == S_OK, "ret %08x\n", r );
    if(r == S_OK)
    {
        r = IXMLDOMNode_get_text( nodeRoot, &str );
        ok( r == S_OK, "ret %08x\n", r );
        ok( compareIgnoreReturnsWhitespace(str, _bstr_("fn1.txt\n\n fn2.txt \n\nf1\n")), "wrong get_text\n");
        SysFreeString(str);

        IXMLDOMNode_Release(nodeRoot);
    }

    if (0) {
    /* this test crashes on win9x */
    r = IXMLDOMNodeList_QueryInterface(node_list, &IID_IDispatch, NULL);
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    }

    r = IXMLDOMNodeList_get_length( node_list, NULL );
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 1, "expect 1 got %d\n", len );

    r = IXMLDOMNodeList_get_item( node_list, 0, NULL );
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    r = IXMLDOMNodeList_nextNode( node_list, NULL );
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    r = IXMLDOMNodeList_get_item( node_list, 0, &node );
    ok( r == S_OK, "ret %08x\n", r ); 
    IXMLDOMNodeList_Release( node_list );

    /* Invalid output parameter*/
    r = IXMLDOMNode_get_text( node, NULL );
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    r = IXMLDOMNode_get_text( node, &str );
    ok( r == S_OK, "ret %08x\n", r );
    ok( !memcmp(str, szfn1_txt, lstrlenW(szfn1_txt) ), "wrong string\n" );
    SysFreeString(str);

    r = IXMLDOMNode_get_attributes( node, &node_map );
    ok( r == S_OK, "ret %08x\n", r );
    
    str = SysAllocString( szvr );
    r = IXMLDOMNamedNodeMap_getNamedItem( node_map, str, &node2 );
    ok( r == S_OK, "ret %08x\n", r );
    SysFreeString(str);

    r = IXMLDOMNode_get_text( node2, &str );
    ok( r == S_OK, "ret %08x\n", r );
    ok( !memcmp(str, szstr2, sizeof(szstr2)), "wrong string\n" );
    SysFreeString(str);

    r = IXMLDOMNode_get_firstChild( node2, &node3 );
    ok( r == S_OK, "ret %08x\n", r );

    r = IXMLDOMNode_get_text( node3, &str );
    ok( r == S_OK, "ret %08x\n", r );
    ok( !memcmp(str, szstr2, sizeof(szstr2)), "wrong string\n" );
    SysFreeString(str);


    IXMLDOMNode_Release( node3 );
    IXMLDOMNode_Release( node2 );
    IXMLDOMNamedNodeMap_Release( node_map );
    IXMLDOMNode_Release( node );
    IXMLDOMDocument_Release( doc );
}

static void test_get_childNodes(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMNode *node, *node2;
    IXMLDOMNodeList *node_list, *node_list2;
    LONG len;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMElement_get_childNodes( element, &node_list );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 4, "len %d\n", len);

    r = IXMLDOMNodeList_get_item( node_list, 2, &node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNode_get_childNodes( node, &node_list2 );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_length( node_list2, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 0, "len %d\n", len);

    r = IXMLDOMNodeList_get_item( node_list2, 0, &node2);
    ok( r == S_FALSE, "ret %08x\n", r);

    IXMLDOMNodeList_Release( node_list2 );
    IXMLDOMNode_Release( node );
    IXMLDOMNodeList_Release( node_list );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );
}

static void test_get_firstChild(void)
{
    static WCHAR xmlW[] = {'x','m','l',0};
    IXMLDOMDocument *doc;
    IXMLDOMNode *node;
    VARIANT_BOOL b;
    HRESULT r;
    BSTR str;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void**)&doc );
    ok( r == S_OK, "failed with 0x%08x\n", r );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_get_firstChild( doc, &node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNode_get_nodeName( node, &str );
    ok( r == S_OK, "ret %08x\n", r);

    ok(memcmp(str, xmlW, sizeof(xmlW)) == 0, "expected \"xml\" node name\n");

    SysFreeString(str);
    IXMLDOMNode_Release( node );
    IXMLDOMDocument_Release( doc );
}

static void test_removeChild(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element, *lc_element;
    IXMLDOMNode *fo_node, *ba_node, *removed_node, *temp_node, *lc_node;
    IXMLDOMNodeList *root_list, *fo_list;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMElement_get_childNodes( element, &root_list );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_item( root_list, 3, &fo_node );
    ok( r == S_OK, "ret %08x\n", r);
 
    r = IXMLDOMNode_get_childNodes( fo_node, &fo_list );
    ok( r == S_OK, "ret %08x\n", r);
 
    r = IXMLDOMNodeList_get_item( fo_list, 0, &ba_node );
    ok( r == S_OK, "ret %08x\n", r);

    /* invalid parameter: NULL ptr */
    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMElement_removeChild( element, NULL, &removed_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    ok( removed_node == (void*)0xdeadbeef, "%p\n", removed_node );

    /* ba_node is a descendant of element, but not a direct child. */
    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMElement_removeChild( element, ba_node, &removed_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    ok( removed_node == NULL, "%p\n", removed_node );

    r = IXMLDOMElement_removeChild( element, fo_node, &removed_node );
    ok( r == S_OK, "ret %08x\n", r);
    ok( fo_node == removed_node, "node %p node2 %p\n", fo_node, removed_node );

    /* try removing already removed child */
    temp_node = (void*)0xdeadbeef;
    r = IXMLDOMElement_removeChild( element, fo_node, &temp_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r);
    ok( temp_node == NULL, "%p\n", temp_node );

    /* the removed node has no parent anymore */
    r = IXMLDOMNode_get_parentNode( removed_node, &temp_node );
    ok( r == S_FALSE, "ret %08x\n", r);
    ok( temp_node == NULL, "%p\n", temp_node );

    IXMLDOMNode_Release( removed_node );
    IXMLDOMNode_Release( ba_node );
    IXMLDOMNodeList_Release( fo_list );

    r = IXMLDOMNodeList_get_item( root_list, 0, &lc_node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMElement_QueryInterface( lc_node, &IID_IXMLDOMElement, (LPVOID*)&lc_element );
    ok( r == S_OK, "ret %08x\n", r);

    /* MS quirk: passing wrong interface pointer works, too */
    r = IXMLDOMElement_removeChild( element, (IXMLDOMNode*)lc_element, NULL );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNode_get_parentNode( lc_node, &temp_node );
    ok( r == S_FALSE, "ret %08x\n", r);
    ok( temp_node == NULL, "%p\n", temp_node );

    IXMLDOMNode_Release( lc_node );
    IXMLDOMNodeList_Release( root_list );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );
}

static void test_replaceChild(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element, *ba_element;
    IXMLDOMNode *fo_node, *ba_node, *lc_node, *removed_node, *temp_node;
    IXMLDOMNodeList *root_list, *fo_list;
    IUnknown * unk1, *unk2;
    LONG len;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMElement_get_childNodes( element, &root_list );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_item( root_list, 0, &lc_node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_item( root_list, 3, &fo_node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNode_get_childNodes( fo_node, &fo_list );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_item( fo_list, 0, &ba_node );
    ok( r == S_OK, "ret %08x\n", r);

    IXMLDOMNodeList_Release( fo_list );

    /* invalid parameter: NULL ptr for element to remove */
    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMElement_replaceChild( element, ba_node, NULL, &removed_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    ok( removed_node == (void*)0xdeadbeef, "%p\n", removed_node );

    /* invalid parameter: NULL for replacement element. (Sic!) */
    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMElement_replaceChild( element, NULL, fo_node, &removed_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    ok( removed_node == (void*)0xdeadbeef, "%p\n", removed_node );

    /* invalid parameter: OldNode is not a child */
    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMElement_replaceChild( element, lc_node, ba_node, &removed_node );
    ok( r == E_INVALIDARG, "ret %08x\n", r );
    ok( removed_node == NULL, "%p\n", removed_node );

    /* invalid parameter: would create loop */
    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMNode_replaceChild( fo_node, fo_node, ba_node, &removed_node );
    ok( r == E_FAIL, "ret %08x\n", r );
    ok( removed_node == NULL, "%p\n", removed_node );

    r = IXMLDOMElement_replaceChild( element, ba_node, fo_node, NULL );
    ok( r == S_OK, "ret %08x\n", r );

    r = IXMLDOMNodeList_get_item( root_list, 3, &temp_node );
    ok( r == S_OK, "ret %08x\n", r );

    /* ba_node and temp_node refer to the same node, yet they
       are different interface pointers */
    ok( ba_node != temp_node, "ba_node %p temp_node %p\n", ba_node, temp_node);
    r = IXMLDOMNode_QueryInterface( temp_node, &IID_IUnknown, (void**)&unk1);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNode_QueryInterface( ba_node, &IID_IUnknown, (void**)&unk2);
    ok( r == S_OK, "ret %08x\n", r );
    todo_wine ok( unk1 == unk2, "unk1 %p unk2 %p\n", unk1, unk2);

    IUnknown_Release( unk1 );
    IUnknown_Release( unk2 );

    /* ba_node should have been removed from below fo_node */
    r = IXMLDOMNode_get_childNodes( fo_node, &fo_list );
    ok( r == S_OK, "ret %08x\n", r );

    /* MS quirk: replaceChild also accepts elements instead of nodes */
    r = IXMLDOMNode_QueryInterface( ba_node, &IID_IXMLDOMElement, (void**)&ba_element);
    ok( r == S_OK, "ret %08x\n", r );

    r = IXMLDOMElement_replaceChild( element, ba_node, (IXMLDOMNode*)ba_element, &removed_node );
    ok( r == S_OK, "ret %08x\n", r );

    r = IXMLDOMNodeList_get_length( fo_list, &len);
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 0, "len %d\n", len);

    IXMLDOMNodeList_Release( fo_list );

    IXMLDOMNode_Release(ba_node);
    IXMLDOMNode_Release(fo_node);
    IXMLDOMNode_Release(temp_node);
    IXMLDOMNodeList_Release( root_list );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );
}

static void test_removeNamedItem(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMNode *pr_node, *removed_node, *removed_node2;
    IXMLDOMNodeList *root_list;
    IXMLDOMNamedNodeMap * pr_attrs;
    VARIANT_BOOL b;
    BSTR str;
    LONG len;
    HRESULT r;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMElement_get_childNodes( element, &root_list );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_item( root_list, 1, &pr_node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNode_get_attributes( pr_node, &pr_attrs );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNamedNodeMap_get_length( pr_attrs, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 3, "length %d\n", len);

    removed_node = (void*)0xdeadbeef;
    r = IXMLDOMNamedNodeMap_removeNamedItem( pr_attrs, NULL, &removed_node);
    ok ( r == E_INVALIDARG, "ret %08x\n", r);
    ok ( removed_node == (void*)0xdeadbeef, "removed_node == %p\n", removed_node);

    removed_node = (void*)0xdeadbeef;
    str = SysAllocString(szvr);
    r = IXMLDOMNamedNodeMap_removeNamedItem( pr_attrs, str, &removed_node);
    ok ( r == S_OK, "ret %08x\n", r);

    removed_node2 = (void*)0xdeadbeef;
    r = IXMLDOMNamedNodeMap_removeNamedItem( pr_attrs, str, &removed_node2);
    ok ( r == S_FALSE, "ret %08x\n", r);
    ok ( removed_node2 == NULL, "removed_node == %p\n", removed_node2 );

    r = IXMLDOMNamedNodeMap_get_length( pr_attrs, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 2, "length %d\n", len);

    r = IXMLDOMNamedNodeMap_setNamedItem( pr_attrs, removed_node, NULL);
    ok ( r == S_OK, "ret %08x\n", r);
    IXMLDOMNode_Release(removed_node);

    r = IXMLDOMNamedNodeMap_get_length( pr_attrs, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 3, "length %d\n", len);

    r = IXMLDOMNamedNodeMap_removeNamedItem( pr_attrs, str, NULL);
    ok ( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNamedNodeMap_get_length( pr_attrs, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 2, "length %d\n", len);

    r = IXMLDOMNamedNodeMap_removeNamedItem( pr_attrs, str, NULL);
    ok ( r == S_FALSE, "ret %08x\n", r);

    SysFreeString(str);

    IXMLDOMNamedNodeMap_Release( pr_attrs );
    IXMLDOMNode_Release( pr_node );
    IXMLDOMNodeList_Release( root_list );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );
}

static void test_XMLHTTP(void)
{
    static const WCHAR wszBody[] = {'m','o','d','e','=','T','e','s','t',0};
    static WCHAR wszPOST[] = {'P','O','S','T',0};
    static WCHAR wszUrl[] = {'h','t','t','p',':','/','/',
        'c','r','o','s','s','o','v','e','r','.','c','o','d','e','w','e','a','v','e','r','s','.','c','o','m','/',
        'p','o','s','t','t','e','s','t','.','p','h','p',0};
    static const WCHAR wszExpectedResponse[] = {'F','A','I','L','E','D',0};
    IXMLHttpRequest *pXMLHttpRequest;
    BSTR bstrResponse, str1, str2;
    VARIANT dummy;
    VARIANT varfalse;
    VARIANT varbody;
    HRESULT hr = CoCreateInstance(&CLSID_XMLHTTPRequest, NULL,
                                  CLSCTX_INPROC_SERVER, &IID_IXMLHttpRequest,
                                  (void **)&pXMLHttpRequest);
    if (FAILED(hr))
    {
        win_skip("IXMLHTTPRequest is not available (0x%08x)\n", hr);
        return;
    }

    VariantInit(&dummy);
    V_VT(&dummy) = VT_ERROR;
    V_ERROR(&dummy) = DISP_E_MEMBERNOTFOUND;
    VariantInit(&varfalse);
    V_VT(&varfalse) = VT_BOOL;
    V_BOOL(&varfalse) = VARIANT_FALSE;
    V_VT(&varbody) = VT_BSTR;
    V_BSTR(&varbody) = SysAllocString(wszBody);

    str1 = SysAllocString(wszPOST);
    str2 = SysAllocString(wszUrl);
    hr = IXMLHttpRequest_open(pXMLHttpRequest, str1, str2, varfalse, dummy, dummy);
    todo_wine ok(hr == S_OK, "IXMLHttpRequest_open should have succeeded instead of failing with 0x%08x\n", hr);
    SysFreeString(str1);
    SysFreeString(str2);

    hr = IXMLHttpRequest_send(pXMLHttpRequest, varbody);
    if (hr == INET_E_RESOURCE_NOT_FOUND)
    {
        skip("No connection could be made with crossover.codeweavers.com\n");
        IXMLHttpRequest_Release(pXMLHttpRequest);
        return;
    }
    todo_wine ok(hr == S_OK, "IXMLHttpRequest_send should have succeeded instead of failing with 0x%08x\n", hr);
    VariantClear(&varbody);

    hr = IXMLHttpRequest_get_responseText(pXMLHttpRequest, &bstrResponse);
    todo_wine ok(hr == S_OK, "IXMLHttpRequest_get_responseText should have succeeded instead of failing with 0x%08x\n", hr);
    /* the server currently returns "FAILED" because the Content-Type header is
     * not what the server expects */
    if(hr == S_OK)
    {
        ok(!memcmp(bstrResponse, wszExpectedResponse, sizeof(wszExpectedResponse)), "bstrResponse differs from what was expected\n");
        SysFreeString(bstrResponse);
    }

    IXMLHttpRequest_Release(pXMLHttpRequest);
}

static void test_IXMLDOMDocument2(void)
{
    HRESULT r;
    VARIANT_BOOL b;
    BSTR str;
    IXMLDOMDocument *doc;
    IXMLDOMDocument2 *doc2;
    IDispatchEx *dispex;
    VARIANT var;
    int ref;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_QueryInterface( doc, &IID_IXMLDOMDocument2, (void**)&doc2 );
    ok( r == S_OK, "ret %08x\n", r );
    ok( doc == (IXMLDOMDocument*)doc2, "interfaces differ\n");

    r = IXMLDOMDocument_QueryInterface( doc, &IID_IDispatchEx, (void**)&dispex );
    ok( r == S_OK, "ret %08x\n", r );
    if(r == S_OK)
    {
        IDispatchEx_Release(dispex);
    }

    /* we will check if the variant got cleared */
    ref = IXMLDOMDocument2_AddRef(doc2);
    expect_eq(ref, 3, int, "%d");  /* doc, doc2, AddRef*/
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown *)doc2;

    /* invalid calls */
    ole_expect(IXMLDOMDocument2_getProperty(doc2, _bstr_("askldhfaklsdf"), &var), E_FAIL);
    expect_eq(V_VT(&var), VT_UNKNOWN, int, "%x");
    ole_expect(IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), NULL), E_INVALIDARG);

    /* valid call */
    ole_check(IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), &var));
    expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    expect_bstr_eq_and_free(V_BSTR(&var), "XSLPattern");
    V_VT(&var) = VT_R4;

    /* the variant didn't get cleared*/
    expect_eq(IXMLDOMDocument2_Release(doc2), 2, int, "%d");

    /* setProperty tests */
    ole_expect(IXMLDOMDocument2_setProperty(doc2, _bstr_("askldhfaklsdf"), var), E_FAIL);
    ole_expect(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), var), E_FAIL);
    ole_expect(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("alskjdh faklsjd hfk")), E_FAIL);
    ole_check(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern")));
    ole_check(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XPath")));
    ole_check(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern")));

    /* contrary to what MSDN claims you can switch back from XPath to XSLPattern */
    ole_check(IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), &var));
    expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    expect_bstr_eq_and_free(V_BSTR(&var), "XSLPattern");

    IXMLDOMDocument2_Release( doc2 );
    IXMLDOMDocument_Release( doc );
    free_bstrs();
}

static void test_XPath(void)
{
    HRESULT r;
    VARIANT var;
    VARIANT_BOOL b;
    IXMLDOMDocument2 *doc;
    IXMLDOMNode *rootNode;
    IXMLDOMNode *elem1Node;
    IXMLDOMNode *node;
    IXMLDOMNodeList *list;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    ole_check(IXMLDOMDocument_loadXML(doc, _bstr_(szExampleXML), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* switch to XPath */
    ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XPath")));

    /* some simple queries*/
    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("root"), &list));
    ole_check(IXMLDOMNodeList_get_item(list, 0, &rootNode));
    ole_check(IXMLDOMNodeList_reset(list));
    expect_list_and_release(list, "E2.D1");
    if (rootNode == NULL)
        return;

    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("root//c"), &list));
    expect_list_and_release(list, "E3.E1.E2.D1 E3.E2.E2.D1");

    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("//c[@type]"), &list));
    expect_list_and_release(list, "E3.E2.E2.D1");

    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("elem"), &list));
    /* using get_item for query results advances the position */
    ole_check(IXMLDOMNodeList_get_item(list, 1, &node));
    expect_node(node, "E2.E2.D1");
    IXMLDOMNode_Release(node);
    ole_check(IXMLDOMNodeList_nextNode(list, &node));
    expect_node(node, "E4.E2.D1");
    IXMLDOMNode_Release(node);
    ole_check(IXMLDOMNodeList_reset(list));
    expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E4.E2.D1");

    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("."), &list));
    expect_list_and_release(list, "E2.D1");

    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("elem[3]/preceding-sibling::*"), &list));
    ole_check(IXMLDOMNodeList_get_item(list, 0, &elem1Node));
    ole_check(IXMLDOMNodeList_reset(list));
    expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E3.E2.D1");

    /* select an attribute */
    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_(".//@type"), &list));
    expect_list_and_release(list, "A'type'.E3.E2.E2.D1");

    /* would evaluate to a number */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_("count(*)"), &list), E_FAIL);
    /* would evaluate to a boolean */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_("position()>0"), &list), E_FAIL);
    /* would evaluate to a string */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_("name()"), &list), E_FAIL);

    /* no results */
    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("c"), &list));
    expect_list_and_release(list, "");
    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("elem//c"), &list));
    expect_list_and_release(list, "");
    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("//elem[4]"), &list));
    expect_list_and_release(list, "");
    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("root//elem[0]"), &list));
    expect_list_and_release(list, "");

    /* foo undeclared in document node */
    ole_expect(IXMLDOMDocument_selectNodes(doc, _bstr_("root//foo:c"), &list), E_FAIL);
    /* undeclared in <root> node */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_(".//foo:c"), &list), E_FAIL);
    /* undeclared in <elem> node */
    ole_expect(IXMLDOMNode_selectNodes(elem1Node, _bstr_("//foo:c"), &list), E_FAIL);
    /* but this trick can be used */
    ole_check(IXMLDOMNode_selectNodes(elem1Node, _bstr_("//*[name()='foo:c']"), &list));
    expect_list_and_release(list, "E3.E4.E2.D1");

    /* it has to be declared in SelectionNamespaces */
    todo_wine ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'")));

    /* now the namespace can be used */
    todo_wine ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("root//test:c"), &list));
    todo_wine expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    todo_wine ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_(".//test:c"), &list));
    todo_wine expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    todo_wine ole_check(IXMLDOMNode_selectNodes(elem1Node, _bstr_("//test:c"), &list));
    todo_wine expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    todo_wine ole_check(IXMLDOMNode_selectNodes(elem1Node, _bstr_(".//test:x"), &list));
    todo_wine expect_list_and_release(list, "E5.E1.E4.E1.E2.D1");

    /* SelectionNamespaces syntax error - the namespaces doesn't work anymore but the value is stored */
    ole_expect(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' xmlns:foo=###")), E_FAIL);

    ole_expect(IXMLDOMDocument_selectNodes(doc, _bstr_("root//foo:c"), &list), E_FAIL);

    VariantInit(&var);
    todo_wine ole_check(IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionNamespaces"), &var));
    todo_wine expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    if (V_VT(&var) == VT_BSTR)
        expect_bstr_eq_and_free(V_BSTR(&var), "xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' xmlns:foo=###");

    /* extra attributes - same thing*/
    ole_expect(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' param='test'")), E_FAIL);
    ole_expect(IXMLDOMDocument_selectNodes(doc, _bstr_("root//foo:c"), &list), E_FAIL);

    IXMLDOMNode_Release(rootNode);
    IXMLDOMNode_Release(elem1Node);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_cloneNode(void )
{
    IXMLDOMDocument *doc = NULL;
    VARIANT_BOOL b;
    IXMLDOMNodeList *pList;
    IXMLDOMNamedNodeMap *mapAttr;
    LONG nLength = 0, nLength1 = 0;
    LONG nAttrCnt = 0, nAttrCnt1 = 0;
    IXMLDOMNode *node;
    IXMLDOMNode *node_clone;
    IXMLDOMNode *node_first;
    HRESULT r;
    BSTR str;
    static const WCHAR szSearch[] = { 'l', 'c', '/', 'p', 'r', 0 };

    r = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    ole_check(IXMLDOMDocument_loadXML(doc, str, &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString(str);

    if(!b)
        return;

    str = SysAllocString( szSearch);
    r = IXMLDOMNode_selectSingleNode(doc, str, &node);
    ok( r == S_OK, "ret %08x\n", r );
    ok( node != NULL, "node %p\n", node );
    SysFreeString(str);

    if(!node)
    {
        IXMLDOMDocument_Release(doc);
        return;
    }

    /* Check invalid parameter */
    r = IXMLDOMNode_cloneNode(node, VARIANT_TRUE, NULL);
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    /* All Children */
    r = IXMLDOMNode_cloneNode(node, VARIANT_TRUE, &node_clone);
    ok( r == S_OK, "ret %08x\n", r );
    ok( node_clone != NULL, "node %p\n", node );

    if(!node_clone)
    {
        IXMLDOMDocument_Release(doc);
        IXMLDOMNode_Release(node);
        return;
    }

    r = IXMLDOMNode_get_firstChild(node_clone, &node_first);
    ok( r == S_OK, "ret %08x\n", r );
    if(r == S_OK)
    {
        IXMLDOMDocument *doc2;

        r = IXMLDOMNode_get_ownerDocument(node_clone, &doc2);
        ok( r == S_OK, "ret %08x\n", r );
        if(r == S_OK)
            IXMLDOMDocument_Release(doc2);

        IXMLDOMNode_Release(node_first);
    }

    r = IXMLDOMNode_get_childNodes(node, &pList);
    ok( r == S_OK, "ret %08x\n", r );
    if (pList)
	{
		IXMLDOMNodeList_get_length(pList, &nLength);
		IXMLDOMNodeList_Release(pList);
	}

    r = IXMLDOMNode_get_attributes(node, &mapAttr);
    ok( r == S_OK, "ret %08x\n", r );
    if(mapAttr)
    {
        IXMLDOMNamedNodeMap_get_length(mapAttr, &nAttrCnt);
        IXMLDOMNamedNodeMap_Release(mapAttr);
    }

    r = IXMLDOMNode_get_childNodes(node_clone, &pList);
    ok( r == S_OK, "ret %08x\n", r );
    if (pList)
	{
		IXMLDOMNodeList_get_length(pList, &nLength1);
		IXMLDOMNodeList_Release(pList);
	}

    r = IXMLDOMNode_get_attributes(node_clone, &mapAttr);
    ok( r == S_OK, "ret %08x\n", r );
    if(mapAttr)
    {
        IXMLDOMNamedNodeMap_get_length(mapAttr, &nAttrCnt1);
        IXMLDOMNamedNodeMap_Release(mapAttr);
    }

    ok(nLength == nLength1, "wrong Child count (%d, %d)\n", nLength, nLength1);
    ok(nAttrCnt == nAttrCnt1, "wrong Attribute count (%d, %d)\n", nAttrCnt, nAttrCnt1);
    IXMLDOMNode_Release(node_clone);

    /* No Children */
    r = IXMLDOMNode_cloneNode(node, VARIANT_FALSE, &node_clone);
    ok( r == S_OK, "ret %08x\n", r );
    ok( node_clone != NULL, "node %p\n", node );

    if(!node_clone)
    {
        IXMLDOMDocument_Release(doc);
        IXMLDOMNode_Release(node);
        return;
    }

    r = IXMLDOMNode_get_firstChild(node_clone, &node_first);
    ok( r == S_FALSE, "ret %08x\n", r );
    if(r == S_OK)
    {
        IXMLDOMDocument *doc2;

        r = IXMLDOMNode_get_ownerDocument(node_clone, &doc2);
        ok( r == S_OK, "ret %08x\n", r );
        if(r == S_OK)
            IXMLDOMDocument_Release(doc2);

        IXMLDOMNode_Release(node_first);
    }

    r = IXMLDOMNode_get_childNodes(node_clone, &pList);
    ok( r == S_OK, "ret %08x\n", r );
    if (pList)
	{
		IXMLDOMNodeList_get_length(pList, &nLength1);
        ok( nLength1 == 0, "Length should be 0 (%d)\n", nLength1);
		IXMLDOMNodeList_Release(pList);
	}

    r = IXMLDOMNode_get_attributes(node_clone, &mapAttr);
    ok( r == S_OK, "ret %08x\n", r );
    if(mapAttr)
    {
        IXMLDOMNamedNodeMap_get_length(mapAttr, &nAttrCnt1);
        ok( nAttrCnt1 == 3, "Attribute count should be 3 (%d)\n", nAttrCnt1);
        IXMLDOMNamedNodeMap_Release(mapAttr);
    }

    ok(nLength != nLength1, "wrong Child count (%d, %d)\n", nLength, nLength1);
    ok(nAttrCnt == nAttrCnt1, "wrong Attribute count (%d, %d)\n", nAttrCnt, nAttrCnt1);
    IXMLDOMNode_Release(node_clone);


    IXMLDOMNode_Release(node);
    IXMLDOMDocument_Release(doc);
}

static void test_xmlTypes(void)
{
    IXMLDOMDocument *doc = NULL;
    IXMLDOMElement *pRoot;
    HRESULT hr;
    IXMLDOMComment *pComment;
    IXMLDOMElement *pElement;
    IXMLDOMAttribute *pAttribute;
    IXMLDOMNamedNodeMap *pAttribs;
    IXMLDOMCDATASection *pCDataSec;
    IXMLDOMImplementation *pIXMLDOMImplementation = NULL;
    IXMLDOMDocumentFragment *pDocFrag = NULL;
    IXMLDOMEntityReference *pEntityRef = NULL;
    BSTR str;
    IXMLDOMNode *pNextChild = (IXMLDOMNode *)0x1;   /* Used for testing Siblings */
    VARIANT v;
    LONG len = 0;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    hr = IXMLDOMDocument_get_nextSibling(doc, NULL);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

    hr = IXMLDOMDocument_get_nextSibling(doc, &pNextChild);
    ok(hr == S_FALSE, "ret %08x\n", hr );
    ok(pNextChild == NULL, "pDocChild not NULL\n");

    /* test previous Sibling */
    hr = IXMLDOMDocument_get_previousSibling(doc, NULL);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

    pNextChild = (IXMLDOMNode *)0x1;
    hr = IXMLDOMDocument_get_previousSibling(doc, &pNextChild);
    ok(hr == S_FALSE, "ret %08x\n", hr );
    ok(pNextChild == NULL, "pNextChild not NULL\n");

    /* test get_attributes */
    hr = IXMLDOMDocument_get_attributes( doc, NULL );
    ok( hr == E_INVALIDARG, "get_attributes returned wrong code\n");

    pAttribs = (IXMLDOMNamedNodeMap*)0x1;
    hr = IXMLDOMDocument_get_attributes( doc, &pAttribs);
    ok(hr == S_FALSE, "ret %08x\n", hr );
    ok( pAttribs == NULL, "pAttribs not NULL\n");

    /* test get_dataType */
    hr = IXMLDOMDocument_get_dataType(doc, &v);
    ok(hr == S_FALSE, "ret %08x\n", hr );
    ok( V_VT(&v) == VT_NULL, "incorrect dataType type\n");
    VariantClear(&v);

    /* test nodeTypeString */
    hr = IXMLDOMDocument_get_nodeTypeString(doc, &str);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok( !lstrcmpW( str, _bstr_("document") ), "incorrect nodeTypeString string\n");
    SysFreeString(str);

    /* test implementation */
    hr = IXMLDOMDocument_get_implementation(doc, NULL);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

    hr = IXMLDOMDocument_get_implementation(doc, &pIXMLDOMImplementation);
    ok(hr == S_OK, "ret %08x\n", hr );
    if(hr == S_OK)
    {
        VARIANT_BOOL hasFeature = VARIANT_TRUE;
        BSTR sEmpty = SysAllocStringLen(NULL, 0);

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, NULL, sEmpty, &hasFeature);
        ok(hr == E_INVALIDARG, "ret %08x\n", hr );

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, sEmpty, sEmpty, NULL);
        ok(hr == E_INVALIDARG, "ret %08x\n", hr );

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), sEmpty, &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned false\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, sEmpty, sEmpty, &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), NULL, &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned false\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), sEmpty, &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned false\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), _bstr_("1.0"), &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("XML"), _bstr_("1.0"), &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("MS-DOM"), _bstr_("1.0"), &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("SSS"), NULL, &hasFeature);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned false\n");

        SysFreeString(sEmpty);
        IXMLDOMImplementation_Release(pIXMLDOMImplementation);
    }

    pRoot = (IXMLDOMElement*)0x1;
    hr = IXMLDOMDocument_createElement(doc, NULL, &pRoot);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );
    ok(pRoot == (void*)0x1, "Expect same ptr, got %p\n", pRoot);

    pRoot = (IXMLDOMElement*)0x1;
    hr = IXMLDOMDocument_createElement(doc, _bstr_(""), &pRoot);
    ok(hr == E_FAIL, "ret %08x\n", hr );
    ok(pRoot == (void*)0x1, "Expect same ptr, got %p\n", pRoot);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &pRoot);
    ok(hr == S_OK, "ret %08x\n", hr );
    if(hr == S_OK)
    {
        hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)pRoot, NULL);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            /* Comment */
            str = SysAllocString(szComment);
            hr = IXMLDOMDocument_createComment(doc, str, &pComment);
            SysFreeString(str);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                /* test get_attributes */
                hr = IXMLDOMComment_get_attributes( pComment, NULL );
                ok( hr == E_INVALIDARG, "get_attributes returned wrong code\n");

                pAttribs = (IXMLDOMNamedNodeMap*)0x1;
                hr = IXMLDOMComment_get_attributes( pComment, &pAttribs);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( pAttribs == NULL, "pAttribs not NULL\n");

                /* test nodeTypeString */
                hr = IXMLDOMComment_get_nodeTypeString(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("comment") ), "incorrect nodeTypeString string\n");
                SysFreeString(str);

                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pComment, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_nodeName(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szCommentNodeText ), "incorrect comment node Name\n");
                SysFreeString(str);

                hr = IXMLDOMComment_get_xml(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szCommentXML ), "incorrect comment xml\n");
                SysFreeString(str);

                hr = IXMLDOMComment_get_dataType(pComment, &v);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( V_VT(&v) == VT_NULL, "incorrect dataType type\n");
                VariantClear(&v);

                /* put data Tests */
                hr = IXMLDOMComment_put_data(pComment, _bstr_("This &is a ; test <>\\"));
                ok(hr == S_OK, "ret %08x\n", hr );

                /* get data Tests */
                hr = IXMLDOMComment_get_data(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect get_data string\n");
                SysFreeString(str);

                /* get data Tests */
                hr = IXMLDOMComment_get_nodeValue(pComment, &v);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( V_VT(&v) == VT_BSTR, "incorrect dataType type\n");
                ok( !lstrcmpW( V_BSTR(&v), _bstr_("This &is a ; test <>\\") ), "incorrect get_nodeValue string\n");
                VariantClear(&v);

                /* Confirm XML text is good */
                hr = IXMLDOMComment_get_xml(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("<!--This &is a ; test <>\\-->") ), "incorrect xml string\n");
                SysFreeString(str);

                /* Confirm we get the put_data Text back */
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect xml string\n");
                SysFreeString(str);

                /* test length property */
                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 21, "expected 21 got %d\n", len);

                /* test substringData */
                hr = IXMLDOMComment_substringData(pComment, 0, 4, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                /* test substringData - Invalid offset */
                str = (BSTR)&szElement;
                hr = IXMLDOMComment_substringData(pComment, -1, 4, &str);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid offset */
                str = (BSTR)&szElement;
                hr = IXMLDOMComment_substringData(pComment, 30, 0, &str);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (BSTR)&szElement;
                hr = IXMLDOMComment_substringData(pComment, 0, -1, &str);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (BSTR)&szElement;
                hr = IXMLDOMComment_substringData(pComment, 2, 0, &str);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Start of string */
                hr = IXMLDOMComment_substringData(pComment, 0, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - Middle of string */
                hr = IXMLDOMComment_substringData(pComment, 13, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("test") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - End of string */
                hr = IXMLDOMComment_substringData(pComment, 20, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("\\") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test appendData */
                hr = IXMLDOMComment_appendData(pComment, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_appendData(pComment, _bstr_(""));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_appendData(pComment, _bstr_("Append"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\Append") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* test insertData */
                str = SysAllocStringLen(NULL, 0);
                hr = IXMLDOMComment_insertData(pComment, -1, str);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, -1, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 1000, str);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 1000, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, str);
                ok(hr == S_OK, "ret %08x\n", hr );
                SysFreeString(str);

                hr = IXMLDOMComment_insertData(pComment, -1, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 1000, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, _bstr_("Begin "));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 17, _bstr_("Middle"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 39, _bstr_(" End"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("Begin This &is a Middle; test <>\\Append End") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* delete data */
                /* invalid arguments */
                hr = IXMLDOMComment_deleteData(pComment, -1, 1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, 0, 0);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, 0, -1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 43, "expected 43 got %d\n", len);

                hr = IXMLDOMComment_deleteData(pComment, len, 1);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, len+1, 1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                /* delete from start */
                hr = IXMLDOMComment_deleteData(pComment, 0, 5);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 38, "expected 38 got %d\n", len);

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_(" This &is a Middle; test <>\\Append End") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* delete from end */
                hr = IXMLDOMComment_deleteData(pComment, 35, 3);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 35, "expected 35 got %d\n", len);

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_(" This &is a Middle; test <>\\Append ") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* delete from inside */
                hr = IXMLDOMComment_deleteData(pComment, 1, 33);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 2, "expected 2 got %d\n", len);

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("  ") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* delete whole data ... */
                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, 0, len);
                ok(hr == S_OK, "ret %08x\n", hr );
                /* ... and try again with empty string */
                hr = IXMLDOMComment_deleteData(pComment, 0, len);
                ok(hr == S_OK, "ret %08x\n", hr );

                /* ::replaceData() */
                V_VT(&v) = VT_BSTR;
                V_BSTR(&v) = SysAllocString(szstr1);
                hr = IXMLDOMComment_put_nodeValue(pComment, v);
                ok(hr == S_OK, "ret %08x\n", hr );
                VariantClear(&v);

                hr = IXMLDOMComment_replaceData(pComment, 6, 0, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string\n");
                SysFreeString(str);

                hr = IXMLDOMComment_replaceData(pComment, 0, 0, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* NULL pointer means delete */
                hr = IXMLDOMComment_replaceData(pComment, 0, 1, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("tr1") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* empty string means delete */
                hr = IXMLDOMComment_replaceData(pComment, 0, 1, _bstr_(""));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("r1") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* zero count means insert */
                hr = IXMLDOMComment_replaceData(pComment, 0, 0, _bstr_("a"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("ar1") ), "incorrect get_text string\n");
                SysFreeString(str);

                hr = IXMLDOMComment_replaceData(pComment, 0, 2, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, _bstr_("m"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("m1") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* nonempty string, count greater than its length */
                hr = IXMLDOMComment_replaceData(pComment, 0, 2, _bstr_("a1.2"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("a1.2") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* nonempty string, count less than its length */
                hr = IXMLDOMComment_replaceData(pComment, 0, 1, _bstr_("wine"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("wine1.2") ), "incorrect get_text string\n");
                SysFreeString(str);

                IXMLDOMComment_Release(pComment);
            }

            /* Element */
            str = SysAllocString(szElement);
            hr = IXMLDOMDocument_createElement(doc, str, &pElement);
            SysFreeString(str);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                /* test nodeTypeString */
                hr = IXMLDOMDocument_get_nodeTypeString(pElement, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("element") ), "incorrect nodeTypeString string\n");
                SysFreeString(str);

                hr = IXMLDOMElement_get_nodeName(pElement, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szElement ), "incorrect element node Name\n");
                SysFreeString(str);

                hr = IXMLDOMElement_get_xml(pElement, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szElementXML ), "incorrect element xml\n");
                SysFreeString(str);

                hr = IXMLDOMElement_get_dataType(pElement, &v);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( V_VT(&v) == VT_NULL, "incorrect dataType type\n");
                VariantClear(&v);

                /* Attribute */
                pAttribute = (IXMLDOMAttribute*)0x1;
                hr = IXMLDOMDocument_createAttribute(doc, NULL, &pAttribute);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                ok(pAttribute == (void*)0x1, "Expect same ptr, got %p\n", pAttribute);

                pAttribute = (IXMLDOMAttribute*)0x1;
                hr = IXMLDOMDocument_createAttribute(doc, _bstr_(""), &pAttribute);
                ok(hr == E_FAIL, "ret %08x\n", hr );
                ok(pAttribute == (void*)0x1, "Expect same ptr, got %p\n", pAttribute);

                str = SysAllocString(szAttribute);
                hr = IXMLDOMDocument_createAttribute(doc, str, &pAttribute);
                SysFreeString(str);
                ok(hr == S_OK, "ret %08x\n", hr );
                if(hr == S_OK)
                {
                    IXMLDOMNode *pNewChild = (IXMLDOMNode *)0x1;

                    hr = IXMLDOMAttribute_get_nextSibling(pAttribute, NULL);
                    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                    pNextChild = (IXMLDOMNode *)0x1;
                    hr = IXMLDOMAttribute_get_nextSibling(pAttribute, &pNextChild);
                    ok(hr == S_FALSE, "ret %08x\n", hr );
                    ok(pNextChild == NULL, "pNextChild not NULL\n");

                    /* test Previous Sibling*/
                    hr = IXMLDOMAttribute_get_previousSibling(pAttribute, NULL);
                    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                    pNextChild = (IXMLDOMNode *)0x1;
                    hr = IXMLDOMAttribute_get_previousSibling(pAttribute, &pNextChild);
                    ok(hr == S_FALSE, "ret %08x\n", hr );
                    ok(pNextChild == NULL, "pNextChild not NULL\n");

                    /* test get_attributes */
                    hr = IXMLDOMAttribute_get_attributes( pAttribute, NULL );
                    ok( hr == E_INVALIDARG, "get_attributes returned wrong code\n");

                    pAttribs = (IXMLDOMNamedNodeMap*)0x1;
                    hr = IXMLDOMAttribute_get_attributes( pAttribute, &pAttribs);
                    ok(hr == S_FALSE, "ret %08x\n", hr );
                    ok( pAttribs == NULL, "pAttribs not NULL\n");

                    hr = IXMLDOMElement_appendChild(pElement, (IXMLDOMNode*)pAttribute, &pNewChild);
                    ok(hr == E_FAIL, "ret %08x\n", hr );
                    ok(pNewChild == NULL, "pNewChild not NULL\n");

                    hr = IXMLDOMElement_get_attributes(pElement, &pAttribs);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    if ( hr == S_OK )
                    {
                        hr = IXMLDOMNamedNodeMap_setNamedItem(pAttribs, (IXMLDOMNode*)pAttribute, NULL );
                        ok(hr == S_OK, "ret %08x\n", hr );

                        IXMLDOMNamedNodeMap_Release(pAttribs);
                    }

                    hr = IXMLDOMAttribute_get_nodeName(pAttribute, &str);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    ok( !lstrcmpW( str, szAttribute ), "incorrect attribute node Name\n");
                    SysFreeString(str);

                    /* test nodeTypeString */
                    hr = IXMLDOMAttribute_get_nodeTypeString(pAttribute, &str);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    ok( !lstrcmpW( str, _bstr_("attribute") ), "incorrect nodeTypeString string\n");
                    SysFreeString(str);

                    /* test nodeName */
                    hr = IXMLDOMAttribute_get_nodeName(pAttribute, &str);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    ok( !lstrcmpW( str, szAttribute ), "incorrect nodeName string\n");
                    SysFreeString(str);

                    /* test name property */
                    hr = IXMLDOMAttribute_get_name(pAttribute, &str);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    ok( !lstrcmpW( str, szAttribute ), "incorrect name string\n");
                    SysFreeString(str);

                    hr = IXMLDOMAttribute_get_xml(pAttribute, &str);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    ok( !lstrcmpW( str, szAttributeXML ), "incorrect attribute xml\n");
                    SysFreeString(str);

                    hr = IXMLDOMAttribute_get_dataType(pAttribute, &v);
                    ok(hr == S_FALSE, "ret %08x\n", hr );
                    ok( V_VT(&v) == VT_NULL, "incorrect dataType type\n");
                    VariantClear(&v);

                    IXMLDOMAttribute_Release(pAttribute);

                    /* Check Element again with the Add Attribute*/
                    hr = IXMLDOMElement_get_xml(pElement, &str);
                    ok(hr == S_OK, "ret %08x\n", hr );
                    ok( !lstrcmpW( str, szElementXML2 ), "incorrect element xml\n");
                    SysFreeString(str);
                }

                hr = IXMLDOMElement_put_text(pElement, _bstr_("TestingNode"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMElement_get_xml(pElement, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szElementXML3 ), "incorrect element xml\n");
                SysFreeString(str);

                /* Test for reversible escaping */
                str = SysAllocString( szStrangeChars );
                hr = IXMLDOMElement_put_text(pElement, str);
                ok(hr == S_OK, "ret %08x\n", hr );
                SysFreeString( str );

                hr = IXMLDOMElement_get_xml(pElement, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szElementXML4 ), "incorrect element xml\n");
                SysFreeString(str);

                hr = IXMLDOMElement_get_text(pElement, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szStrangeChars ), "incorrect element text\n");
                SysFreeString(str);

                IXMLDOMElement_Release(pElement);
            }

            /* CData Section */
            str = SysAllocString(szCData);
            hr = IXMLDOMDocument_createCDATASection(doc, str, NULL);
            ok(hr == E_INVALIDARG, "ret %08x\n", hr );

            hr = IXMLDOMDocument_createCDATASection(doc, str, &pCDataSec);
            SysFreeString(str);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMNode *pNextChild = (IXMLDOMNode *)0x1;
                VARIANT var;

                VariantInit(&var);

                hr = IXMLDOMCDATASection_QueryInterface(pCDataSec, &IID_IXMLDOMElement, (LPVOID*)&pElement);
                ok(hr == E_NOINTERFACE, "ret %08x\n", hr);

                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pCDataSec, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                /* get Attribute Tests */
                hr = IXMLDOMCDATASection_get_attributes(pCDataSec, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                pAttribs = (IXMLDOMNamedNodeMap*)0x1;
                hr = IXMLDOMCDATASection_get_attributes(pCDataSec, &pAttribs);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok(pAttribs == NULL, "pAttribs != NULL\n");

                hr = IXMLDOMCDATASection_get_nodeName(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szCDataNodeText ), "incorrect cdata node Name\n");
                SysFreeString(str);

                hr = IXMLDOMCDATASection_get_xml(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szCDataXML ), "incorrect cdata xml\n");
                SysFreeString(str);

                /* test lastChild */
                pNextChild = (IXMLDOMNode*)0x1;
                hr = IXMLDOMCDATASection_get_lastChild(pCDataSec, &pNextChild);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok(pNextChild == NULL, "pNextChild not NULL\n");

                /* test get_dataType */
                hr = IXMLDOMCDATASection_get_dataType(pCDataSec, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_dataType(pCDataSec, &v);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( V_VT(&v) == VT_NULL, "incorrect dataType type\n");
                VariantClear(&v);

                /* test nodeTypeString */
                hr = IXMLDOMCDATASection_get_nodeTypeString(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("cdatasection") ), "incorrect nodeTypeString string\n");
                SysFreeString(str);

                /* put data Tests */
                hr = IXMLDOMCDATASection_put_data(pCDataSec, _bstr_("This &is a ; test <>\\"));
                ok(hr == S_OK, "ret %08x\n", hr );

                /* Confirm XML text is good */
                hr = IXMLDOMCDATASection_get_xml(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("<![CDATA[This &is a ; test <>\\]]>") ), "incorrect xml string\n");
                SysFreeString(str);

                /* Confirm we get the put_data Text back */
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect text string\n");
                SysFreeString(str);

                /* test length property */
                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 21, "expected 21 got %d\n", len);

                /* test get nodeValue */
                hr = IXMLDOMCDATASection_get_nodeValue(pCDataSec, &var);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(V_VT(&var) == VT_BSTR, "got vt %04x\n", V_VT(&var));
                ok( !lstrcmpW( V_BSTR(&var), _bstr_("This &is a ; test <>\\") ), "incorrect text string\n");
                VariantClear(&var);

                /* test get data */
                hr = IXMLDOMCDATASection_get_data(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\") ), "incorrect text string\n");
                SysFreeString(str);

                /* test substringData */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 0, 4, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                /* test substringData - Invalid offset */
                str = (BSTR)&szElement;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, -1, 4, &str);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid offset */
                str = (BSTR)&szElement;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 30, 0, &str);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (BSTR)&szElement;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 0, -1, &str);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (BSTR)&szElement;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 2, 0, &str);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Start of string */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 0, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - Middle of string */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 13, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("test") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - End of string */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 20, 4, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("\\") ), "incorrect substringData string\n");
                SysFreeString(str);

                /* test appendData */
                hr = IXMLDOMCDATASection_appendData(pCDataSec, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_appendData(pCDataSec, _bstr_(""));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_appendData(pCDataSec, _bstr_("Append"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("This &is a ; test <>\\Append") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* test insertData */
                str = SysAllocStringLen(NULL, 0);
                hr = IXMLDOMCDATASection_insertData(pCDataSec, -1, str);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, -1, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 1000, str);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 1000, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, str);
                ok(hr == S_OK, "ret %08x\n", hr );
                SysFreeString(str);

                hr = IXMLDOMCDATASection_insertData(pCDataSec, -1, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 1000, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, _bstr_("Begin "));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 17, _bstr_("Middle"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 39, _bstr_(" End"));
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("Begin This &is a Middle; test <>\\Append End") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* delete data */
                /* invalid arguments */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, -1, 1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, 0);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, -1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 43, "expected 43 got %d\n", len);

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, len, 1);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, len+1, 1);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                /* delete from start */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, 5);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 38, "expected 38 got %d\n", len);

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_(" This &is a Middle; test <>\\Append End") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* delete from end */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 35, 3);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 35, "expected 35 got %d\n", len);

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_(" This &is a Middle; test <>\\Append ") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* delete from inside */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 1, 33);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok(len == 2, "expected 2 got %d\n", len);

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("  ") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* delete whole data ... */
                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, len);
                ok(hr == S_OK, "ret %08x\n", hr );

                /* ... and try again with empty string */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, len);
                ok(hr == S_OK, "ret %08x\n", hr );

                /* ::replaceData() */
                V_VT(&v) = VT_BSTR;
                V_BSTR(&v) = SysAllocString(szstr1);
                hr = IXMLDOMCDATASection_put_nodeValue(pCDataSec, v);
                ok(hr == S_OK, "ret %08x\n", hr );
                VariantClear(&v);

                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 6, 0, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string\n");
                SysFreeString(str);

                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 0, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("str1") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* NULL pointer means delete */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 1, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("tr1") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* empty string means delete */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 1, _bstr_(""));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("r1") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* zero count means insert */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 0, _bstr_("a"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("ar1") ), "incorrect get_text string\n");
                SysFreeString(str);

                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 2, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, _bstr_("m"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("m1") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* nonempty string, count greater than its length */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 2, _bstr_("a1.2"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("a1.2") ), "incorrect get_text string\n");
                SysFreeString(str);

                /* nonempty string, count less than its length */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 1, _bstr_("wine"));
                ok(hr == S_OK, "ret %08x\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("wine1.2") ), "incorrect get_text string\n");
                SysFreeString(str);

                IXMLDOMCDATASection_Release(pCDataSec);
            }

            /* Document Fragments */
            hr = IXMLDOMDocument_createDocumentFragment(doc, NULL);
            ok(hr == E_INVALIDARG, "ret %08x\n", hr );

            hr = IXMLDOMDocument_createDocumentFragment(doc, &pDocFrag);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMNode *node;

                hr = IXMLDOMDocumentFragment_get_parentNode(pDocFrag, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                node = (IXMLDOMNode *)0x1;
                hr = IXMLDOMDocumentFragment_get_parentNode(pDocFrag, &node);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok(node == NULL, "expected NULL, got %p\n", node);

                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pDocFrag, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                /* get Attribute Tests */
                hr = IXMLDOMDocumentFragment_get_attributes(pDocFrag, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                pAttribs = (IXMLDOMNamedNodeMap*)0x1;
                hr = IXMLDOMDocumentFragment_get_attributes(pDocFrag, &pAttribs);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok(pAttribs == NULL, "pAttribs != NULL\n");

                hr = IXMLDOMDocumentFragment_get_nodeName(pDocFrag, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szDocFragmentText ), "incorrect docfragment node Name\n");
                SysFreeString(str);

                /* test next Sibling*/
                hr = IXMLDOMDocumentFragment_get_nextSibling(pDocFrag, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                node = (IXMLDOMNode *)0x1;
                hr = IXMLDOMDocumentFragment_get_nextSibling(pDocFrag, &node);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok(node == NULL, "next sibling not NULL\n");

                /* test Previous Sibling*/
                hr = IXMLDOMDocumentFragment_get_previousSibling(pDocFrag, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                node = (IXMLDOMNode *)0x1;
                hr = IXMLDOMDocumentFragment_get_previousSibling(pDocFrag, &node);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok(node == NULL, "previous sibling not NULL\n");

                /* test get_dataType */
                hr = IXMLDOMDocumentFragment_get_dataType(pDocFrag, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                hr = IXMLDOMDocumentFragment_get_dataType(pDocFrag, &v);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( V_VT(&v) == VT_NULL, "incorrect dataType type\n");
                VariantClear(&v);

                /* test nodeTypeString */
                hr = IXMLDOMDocumentFragment_get_nodeTypeString(pDocFrag, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("documentfragment") ), "incorrect nodeTypeString string\n");
                SysFreeString(str);

                IXMLDOMDocumentFragment_Release(pDocFrag);
            }

            /* Entity References */
            hr = IXMLDOMDocument_createEntityReference(doc, NULL, &pEntityRef);
            ok(hr == E_FAIL, "ret %08x\n", hr );
            hr = IXMLDOMDocument_createEntityReference(doc, _bstr_(""), &pEntityRef);
            ok(hr == E_FAIL, "ret %08x\n", hr );

            str = SysAllocString(szEntityRef);
            hr = IXMLDOMDocument_createEntityReference(doc, str, NULL);
            ok(hr == E_INVALIDARG, "ret %08x\n", hr );

            hr = IXMLDOMDocument_createEntityReference(doc, str, &pEntityRef);
            SysFreeString(str);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pEntityRef, NULL);
                ok(hr == S_OK, "ret %08x\n", hr );

                /* get Attribute Tests */
                hr = IXMLDOMEntityReference_get_attributes(pEntityRef, NULL);
                ok(hr == E_INVALIDARG, "ret %08x\n", hr );

                pAttribs = (IXMLDOMNamedNodeMap*)0x1;
                hr = IXMLDOMEntityReference_get_attributes(pEntityRef, &pAttribs);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok(pAttribs == NULL, "pAttribs != NULL\n");

                /* test dataType */
                hr = IXMLDOMEntityReference_get_dataType(pEntityRef, &v);
                ok(hr == S_FALSE, "ret %08x\n", hr );
                ok( V_VT(&v) == VT_NULL, "incorrect dataType type\n");
                VariantClear(&v);

                /* test nodeTypeString */
                hr = IXMLDOMEntityReference_get_nodeTypeString(pEntityRef, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, _bstr_("entityreference") ), "incorrect nodeTypeString string\n");
                SysFreeString(str);

                /* test get_xml*/
                hr = IXMLDOMEntityReference_get_xml(pEntityRef, &str);
                ok(hr == S_OK, "ret %08x\n", hr );
                ok( !lstrcmpW( str, szEntityRefXML ), "incorrect xml string\n");
                SysFreeString(str);

                IXMLDOMEntityReference_Release(pEntityRef);
            }

            IXMLDOMElement_Release( pRoot );
        }
    }

    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_nodeTypeTests( void )
{
    IXMLDOMDocument *doc = NULL;
    IXMLDOMElement *pRoot;
    IXMLDOMElement *pElement;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), NULL);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr );

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &pRoot);
    ok(hr == S_OK, "ret %08x\n", hr );
    if(hr == S_OK)
    {
        hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)pRoot, NULL);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMElement_put_dataType(pRoot, NULL);
            ok(hr == E_INVALIDARG, "ret %08x\n", hr );

            /* Invalid Value */
            hr = IXMLDOMElement_put_dataType(pRoot, _bstr_("abcdefg") );
            ok(hr == E_FAIL, "ret %08x\n", hr );

            /* NOTE:
             *   The name passed into put_dataType is case-insensitive. So many of the names
             *     have been changed to reflect this.
             */
            /* Boolean */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Boolean"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("Boolean") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* String */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_String"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("String") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Number */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Number"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("number") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Int */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Int"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("InT") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Fixed */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Fixed"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("fixed.14.4") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* DateTime */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_DateTime"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("DateTime") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* DateTime TZ */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_DateTime_tz"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("DateTime.tz") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Date */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Date"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("Date") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Time */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Time"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("Time") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Time.tz */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Time_TZ"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("Time.tz") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* I1 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_I1"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("I1") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* I2 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_I2"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("I2") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* I4 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_I4"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("I4") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* UI1 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_UI1"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("UI1") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* UI2 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_UI2"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("UI2") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* UI4 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_UI4"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("UI4") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* r4 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_r4"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("r4") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* r8 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_r8"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("r8") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* float */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_float"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("float") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* uuid */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_uuid"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("UuId") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* bin.hex */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_bin_hex"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("bin.hex") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* bin.base64 */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_bin_base64"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("bin.base64") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            /* Check changing types */
            hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Change"), &pElement);
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("DateTime.tz") );
                ok(hr == S_OK, "ret %08x\n", hr );

                hr = IXMLDOMElement_put_dataType(pElement, _bstr_("string") );
                ok(hr == S_OK, "ret %08x\n", hr );

                IXMLDOMElement_Release(pElement);
            }

            IXMLDOMElement_Release(pRoot);
        }
    }

    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_DocumentSaveToDocument(void)
{
    IXMLDOMDocument *doc = NULL;
    IXMLDOMDocument *doc2 = NULL;
    IXMLDOMElement *pRoot;

    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc2 );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
    {
        IXMLDOMDocument_Release(doc);
        return;
    }

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &pRoot);
    ok(hr == S_OK, "ret %08x\n", hr );
    if(hr == S_OK)
    {
        hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)pRoot, NULL);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            VARIANT vDoc;
            BSTR sOrig;
            BSTR sNew;

            V_VT(&vDoc) = VT_UNKNOWN;
            V_UNKNOWN(&vDoc) = (IUnknown*)doc2;

            hr = IXMLDOMDocument_save(doc, vDoc);
            ok(hr == S_OK, "ret %08x\n", hr );

            hr = IXMLDOMDocument_get_xml(doc, &sOrig);
            ok(hr == S_OK, "ret %08x\n", hr );

            hr = IXMLDOMDocument_get_xml(doc2, &sNew);
            ok(hr == S_OK, "ret %08x\n", hr );

            ok( !lstrcmpW( sOrig, sNew ), "New document is not the same as origial\n");

            SysFreeString(sOrig);
            SysFreeString(sNew);
        }
    }

    IXMLDOMDocument_Release(doc2);
    IXMLDOMDocument_Release(doc);
}

static void test_DocumentSaveToFile(void)
{
    IXMLDOMDocument *doc = NULL;
    IXMLDOMElement *pRoot;
    HANDLE file;
    char buffer[100];
    DWORD read = 0;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &pRoot);
    ok(hr == S_OK, "ret %08x\n", hr );
    if(hr == S_OK)
    {
        hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)pRoot, NULL);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            VARIANT vFile;

            V_VT(&vFile) = VT_BSTR;
            V_BSTR(&vFile) = _bstr_("test.xml");

            hr = IXMLDOMDocument_save(doc, vFile);
            ok(hr == S_OK, "ret %08x\n", hr );
        }
    }

    IXMLDOMElement_Release(pRoot);
    IXMLDOMDocument_Release(doc);

    file = CreateFile("test.xml", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(file != INVALID_HANDLE_VALUE, "Could not open file: %u\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return;

    ReadFile(file, buffer, sizeof(buffer), &read, NULL);
    ok(read != 0, "could not read file\n");
    ok(buffer[0] != '<' || buffer[1] != '?', "File contains processing instruction\n");

    CloseHandle(file);
    DeleteFile("test.xml");
}

static void test_testTransforms(void)
{
    IXMLDOMDocument *doc = NULL;
    IXMLDOMDocument *docSS = NULL;
    IXMLDOMNode *pNode;
    VARIANT_BOOL bSucc;

    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&docSS );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
    {
        IXMLDOMDocument_Release(doc);
        return;
    }

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szTransformXML), &bSucc);
    ok(hr == S_OK, "ret %08x\n", hr );

    hr = IXMLDOMDocument_loadXML(docSS, _bstr_(szTransformSSXML), &bSucc);
    ok(hr == S_OK, "ret %08x\n", hr );

    hr = IXMLDOMDocument_QueryInterface(docSS, &IID_IXMLDOMNode, (LPVOID*)&pNode );
    ok(hr == S_OK, "ret %08x\n", hr );
    if(hr == S_OK)
    {
        BSTR bOut;

        hr = IXMLDOMDocument_transformNode(doc, pNode, &bOut);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok( compareIgnoreReturns( bOut, _bstr_(szTransformOutput)), "Stylesheet output not correct\n");
        SysFreeString(bOut);

        IXMLDOMNode_Release(pNode);
    }

    IXMLDOMDocument_Release(docSS);
    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_Namespaces(void)
{
    IXMLDOMDocument2 *doc = NULL;
    IXMLDOMNode *pNode;
    IXMLDOMNode *pNode2 = NULL;
    VARIANT_BOOL bSucc;
    HRESULT hr;
    BSTR str;
    static  const CHAR szNamespacesXML[] =
"<?xml version=\"1.0\"?>\n"
"<root xmlns:WEB='http://www.winehq.org'>\n"
"<WEB:Site version=\"1.0\" />\n"
"</root>";

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szNamespacesXML), &bSucc);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(bSucc == VARIANT_TRUE, "Expected VARIANT_TRUE got VARIANT_FALSE\n");

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("root"), &pNode );
    ok(hr == S_OK, "ret %08x\n", hr );
    if(hr == S_OK)
    {
        hr = IXMLDOMNode_get_firstChild( pNode, &pNode2 );
        ok( hr == S_OK, "ret %08x\n", hr );
        ok( pNode2 != NULL, "pNode2 == NULL\n");

        /* Test get_prefix */
        hr = IXMLDOMNode_get_prefix(pNode2, NULL);
        ok( hr == E_INVALIDARG, "ret %08x\n", hr );
        /* NOTE: Need to test that arg2 gets cleared on Error. */

        hr = IXMLDOMNode_get_prefix(pNode2, &str);
        ok( hr == S_OK, "ret %08x\n", hr );
        ok( !lstrcmpW( str, _bstr_("WEB")), "incorrect prefix string\n");
        SysFreeString(str);

        /* Test get_namespaceURI */
        hr = IXMLDOMNode_get_namespaceURI(pNode2, NULL);
        ok( hr == E_INVALIDARG, "ret %08x\n", hr );
        /* NOTE: Need to test that arg2 gets cleared on Error. */

        hr = IXMLDOMNode_get_namespaceURI(pNode2, &str);
        ok( hr == S_OK, "ret %08x\n", hr );
        ok( !lstrcmpW( str, _bstr_("http://www.winehq.org")), "incorrect namespaceURI string\n");
        SysFreeString(str);

        IXMLDOMNode_Release(pNode2);
        IXMLDOMNode_Release(pNode);
    }

    IXMLDOMDocument2_Release(doc);

    free_bstrs();
}

static void test_FormattingXML(void)
{
    IXMLDOMDocument2 *doc = NULL;
    IXMLDOMElement *pElement;
    VARIANT_BOOL bSucc;
    HRESULT hr;
    BSTR str;
    static const CHAR szLinefeedXML[] = "<?xml version=\"1.0\"?>\n<Root>\n\t<Sub val=\"A\" />\n</Root>";
    static const CHAR szLinefeedRootXML[] = "<Root>\r\n\t<Sub val=\"A\"/>\r\n</Root>";

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szLinefeedXML), &bSucc);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(bSucc == VARIANT_TRUE, "Expected VARIANT_TRUE got VARIANT_FALSE\n");

    if(bSucc == VARIANT_TRUE)
    {
        hr = IXMLDOMDocument2_get_documentElement(doc, &pElement);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMElement_get_xml(pElement, &str);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok( !lstrcmpW( str, _bstr_(szLinefeedRootXML) ), "incorrect element xml\n");
            SysFreeString(str);

            IXMLDOMElement_Release(pElement);
        }
    }

    IXMLDOMDocument2_Release(doc);

    free_bstrs();
}

static void test_NodeTypeValue(void)
{
    IXMLDOMDocument2 *doc = NULL;
    IXMLDOMNode *pNode;
    VARIANT_BOOL bSucc;
    HRESULT hr;
    VARIANT v;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szTypeValueXML), &bSucc);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(bSucc == VARIANT_TRUE, "Expected VARIANT_TRUE got VARIANT_FALSE\n");
    if(bSucc == VARIANT_TRUE)
    {
        hr = IXMLDOMDocument2_get_nodeValue(doc, NULL);
        ok(hr == E_INVALIDARG, "ret %08x\n", hr );

        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = NULL;
        hr = IXMLDOMDocument2_get_nodeValue(doc, &v);
        ok(hr == S_FALSE, "ret %08x\n", hr );
        ok(V_VT(&v) == VT_NULL, "expect VT_NULL got %d\n", V_VT(&v));

        hr = IXMLDOMDocument2_get_nodeTypedValue(doc, NULL);
        ok(hr == E_INVALIDARG, "ret %08x\n", hr );

        hr = IXMLDOMDocument2_get_nodeTypedValue(doc, &v);
        ok(hr == S_FALSE, "ret %08x\n", hr );

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/string"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            V_VT(&v) = VT_BSTR;
            V_BSTR(&v) = NULL;
            hr = IXMLDOMNode_get_nodeValue(pNode, &v);
            ok(hr == S_FALSE, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_NULL, "expect VT_NULL got %d\n", V_VT(&v));

            hr = IXMLDOMNode_get_nodeTypedValue(pNode, NULL);
            ok(hr == E_INVALIDARG, "ret %08x\n", hr );

            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_BSTR, "incorrect type\n");
            ok(!lstrcmpW( V_BSTR(&v), _bstr_("Wine") ), "incorrect value\n");
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/string2"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_BSTR, "incorrect type\n");
            ok(!lstrcmpW( V_BSTR(&v), _bstr_("String") ), "incorrect value\n");
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/number"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_BSTR, "incorrect type\n");
            ok(!lstrcmpW( V_BSTR(&v), _bstr_("12.44") ), "incorrect value\n");
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/number2"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_BSTR, "incorrect type\n");
            ok(!lstrcmpW( V_BSTR(&v), _bstr_("-3.71e3") ), "incorrect value\n");
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/int"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_I4, "incorrect type\n");
            ok(V_I4(&v) == -13, "incorrect value\n");
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/fixed"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_CY, "incorrect type\n");
            VariantClear(&v);

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/bool"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_BOOL, "incorrect type\n");
            ok(V_BOOL(&v) == VARIANT_TRUE, "incorrect value\n");
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/datetime"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_DATE, "incorrect type\n");
            double_eq(40135.13996527778, V_DATE(&v));
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/datetimetz"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_DATE, "incorrect type\n");
            double_eq(37813.59302083334, V_DATE(&v));
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/date"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_DATE, "incorrect type\n");
            double_eq(665413.0, V_DATE(&v));
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/time"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_DATE, "incorrect type\n");
            double_eq(0.5813888888888888, V_DATE(&v));
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/timetz"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_DATE, "incorrect type\n");
            double_eq(1.112511574074074, V_DATE(&v));
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/i1"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_I1, "incorrect type\n");
            ok(V_I1(&v) == -13, "incorrect value\n");
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/i2"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_I2, "incorrect type\n");
            ok(V_I2(&v) == 31915, "incorrect value\n");
            VariantClear( &v );

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/i4"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_I4, "incorrect type\n");
            ok(V_I4(&v) == -312232, "incorrect value\n");
            VariantClear(&v);

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/ui1"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_UI1, "incorrect type\n");
            ok(V_UI1(&v) == 123, "incorrect value\n");
            VariantClear(&v);

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/ui2"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_UI2, "incorrect type\n");
            ok(V_UI2(&v) == 48282, "incorrect value\n");
            VariantClear(&v);

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/ui4"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_UI4, "incorrect type\n");
            ok(V_UI4(&v) == 949281, "incorrect value\n");
            VariantClear(&v);

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/r4"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_R4, "incorrect type\n");
            double_eq(213124.0, V_R4(&v));
            VariantClear(&v);

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/r8"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_R8, "incorrect type\n");
            double_eq(0.412, V_R8(&v));
            VariantClear(&v);

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/float"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_R8, "incorrect type\n");
            double_eq(41221.421, V_R8(&v));
            VariantClear(&v);

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/uuid"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == VT_BSTR, "incorrect type\n");
            ok(!lstrcmpW(V_BSTR(&v), _bstr_("333C7BC4-460F-11D0-BC04-0080C7055a83")), "incorrect value\n");
            VariantClear(&v);

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/binhex"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            BYTE bytes[] = {0xff,0xfc,0xa0,0x12,0x00,0x3c};

            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == (VT_ARRAY|VT_UI1), "incorrect type\n");
            ok(V_ARRAY(&v)->rgsabound[0].cElements == 6, "incorrect array size\n");
            if(V_ARRAY(&v)->rgsabound[0].cElements == 6)
                ok(!memcmp(bytes, V_ARRAY(&v)->pvData, sizeof(bytes)), "incorrect value\n");
            VariantClear(&v);

            IXMLDOMNode_Release(pNode);
        }

        hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("root/binbase64"), &pNode);
        ok(hr == S_OK, "ret %08x\n", hr );
        if(hr == S_OK)
        {
            BYTE bytes[] = {0x62,0x61,0x73,0x65,0x36,0x34,0x20,0x74,0x65,0x73,0x74};

            hr = IXMLDOMNode_get_nodeTypedValue(pNode, &v);
            ok(hr == S_OK, "ret %08x\n", hr );
            ok(V_VT(&v) == (VT_ARRAY|VT_UI1), "incorrect type\n");
            ok(V_ARRAY(&v)->rgsabound[0].cElements == 11, "incorrect array size\n");
            if(V_ARRAY(&v)->rgsabound[0].cElements == 11)
                ok(!memcmp(bytes, V_ARRAY(&v)->pvData, sizeof(bytes)), "incorrect value\n");
            VariantClear(&v);

            IXMLDOMNode_Release(pNode);
        }
    }

    IXMLDOMDocument2_Release(doc);

    free_bstrs();
}

static void test_TransformWithLoadingLocalFile(void)
{
    IXMLDOMDocument2 *doc = NULL;
    IXMLDOMDocument2 *xsl = NULL;
    IXMLDOMNode *pNode;
    VARIANT_BOOL bSucc;
    HRESULT hr;
    HANDLE file;
    DWORD dwWritten;
    char lpPathBuffer[MAX_PATH];
    int i;

    /* Create a Temp File. */
    GetTempPathA(MAX_PATH, lpPathBuffer);
    strcat(lpPathBuffer, "customers.xml" );

    file = CreateFile(lpPathBuffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(file != INVALID_HANDLE_VALUE, "Could not create file: %u\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return;

    WriteFile(file, szBasicTransformXML, strlen(szBasicTransformXML), &dwWritten, NULL);
    CloseHandle(file);

    /* Correct path to not include a escape character. */
    for(i=0; i < strlen(lpPathBuffer); i++)
    {
        if(lpPathBuffer[i] == '\\')
            lpPathBuffer[i] = '/';
    }

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&xsl );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
    {
        IXMLDOMDocument2_Release(doc);
        return;
    }

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szTypeValueXML), &bSucc);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(bSucc == VARIANT_TRUE, "Expected VARIANT_TRUE got VARIANT_FALSE\n");
    if(bSucc == VARIANT_TRUE)
    {
        BSTR sXSL;
        BSTR sPart1 = _bstr_(szBasicTransformSSXMLPart1);
        BSTR sPart2 = _bstr_(szBasicTransformSSXMLPart2);
        BSTR sFileName = _bstr_(lpPathBuffer);
        int nLegnth = lstrlenW(sPart1) + lstrlenW(sPart2) + lstrlenW(sFileName) + 1;

        sXSL = SysAllocStringLen(NULL, nLegnth);
        lstrcpyW(sXSL, sPart1);
        lstrcatW(sXSL, sFileName);
        lstrcatW(sXSL, sPart2);

        hr = IXMLDOMDocument2_loadXML(xsl, sXSL, &bSucc);
        ok(hr == S_OK, "ret %08x\n", hr );
        ok(bSucc == VARIANT_TRUE, "Expected VARIANT_TRUE got VARIANT_FALSE\n");
        if(bSucc == VARIANT_TRUE)
        {
            BSTR sResult;

            hr = IXMLDOMDocument_QueryInterface(xsl, &IID_IXMLDOMNode, (LPVOID*)&pNode );
            ok(hr == S_OK, "ret %08x\n", hr );
            if(hr == S_OK)
            {
                /* This will load the temp file via the XSL */
                hr = IXMLDOMDocument2_transformNode(doc, pNode, &sResult);
                ok(hr == S_OK, "ret %08x\n", hr );
                if(hr == S_OK)
                {
                    ok( compareIgnoreReturns( sResult, _bstr_(szBasicTransformOutput)), "Stylesheet output not correct\n");
                    SysFreeString(sResult);
                }

                IXMLDOMNode_Release(pNode);
            }
        }

        SysFreeString(sXSL);
    }

    IXMLDOMDocument2_Release(doc);
    IXMLDOMDocument2_Release(xsl);

    DeleteFile(lpPathBuffer);
    free_bstrs();
}

static void test_put_nodeValue(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMEntityReference *entityref;
    IXMLDOMNode *node;
    HRESULT hr;
    VARIANT data, type;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    /* test for unsupported types */
    /* NODE_DOCUMENT */
    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "ret %08x\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "ret %08x\n", hr );
    IXMLDOMNode_Release(node);

    /* NODE_DOCUMENT_FRAGMENT */
    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_DOCUMENT_FRAGMENT;
    hr = IXMLDOMDocument_createNode(doc, type, _bstr_("test"), NULL, &node);
    ok(hr == S_OK, "ret %08x\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "ret %08x\n", hr );
    IXMLDOMNode_Release(node);

    /* NODE_ELEMENT */
    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ELEMENT;
    hr = IXMLDOMDocument_createNode(doc, type, _bstr_("test"), NULL, &node);
    ok(hr == S_OK, "ret %08x\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "ret %08x\n", hr );
    IXMLDOMNode_Release(node);

    /* NODE_ENTITY_REFERENCE */
    hr = IXMLDOMDocument_createEntityReference(doc, _bstr_("ref"), &entityref);
    ok(hr == S_OK, "ret %08x\n", hr );

    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMEntityReference_put_nodeValue(entityref, data);
    ok(hr == E_FAIL, "ret %08x\n", hr );

    hr = IXMLDOMEntityReference_QueryInterface(entityref, &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "ret %08x\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "ret %08x\n", hr );
    IXMLDOMNode_Release(node);
    IXMLDOMEntityReference_Release(entityref);

    free_bstrs();

    IXMLDOMDocument_Release(doc);
}

static void test_IObjectSafety_set(IObjectSafety *safety, HRESULT result, HRESULT result2, DWORD set, DWORD mask, DWORD expected, DWORD expected2)
{
    HRESULT hr;
    DWORD enabled, supported;

    trace("testing IObjectSafety: enable=%08x, mask=%08x\n", set, mask);

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL, set, mask);
    if (result == result2)
        ok(hr == result, "SetInterfaceSafetyOptions: expected %08x, returned %08x\n", result, hr );
    else
        ok(broken(hr == result) || hr == result2,
           "SetInterfaceSafetyOptions: expected %08x, got %08x\n", result2, hr );

    supported = enabled = 0xCAFECAFE;
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok(hr == S_OK, "ret %08x\n", hr );
    if (expected == expected2)
        ok(enabled == expected, "Expected %08x, got %08x\n", expected, enabled);
    else
        ok(broken(enabled == expected) || enabled == expected2,
           "Expected %08x, got %08x\n", expected2, enabled);

    /* reset the safety options */

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL,
            INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_SECURITY_MANAGER,
            0);
    ok(hr == S_OK, "ret %08x\n", hr );

    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(enabled == 0, "Expected 0, got %08x\n", enabled);
}

static void test_document_IObjectSafety(void)
{
    IXMLDOMDocument *doc;
    IObjectSafety *safety;
    DWORD enabled = 0, supported = 0;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    ok( hr == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", hr );
    if( hr != S_OK )
        return;

    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IObjectSafety, (void**)&safety);
    ok(hr == S_OK, "ret %08x\n", hr );

    /* get */
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, NULL, &enabled);
    ok(hr == E_POINTER, "ret %08x\n", hr );
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, NULL);
    ok(hr == E_POINTER, "ret %08x\n", hr );

    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok(hr == S_OK, "ret %08x\n", hr );
    ok(broken(supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA)) ||
       supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER) /* msxml3 SP8+ */,
        "Expected (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER), "
             "got %08x\n", supported);
    ok(enabled == 0, "Expected 0, got %08x\n", enabled);

    /* set -- individual flags */

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER, INTERFACESAFE_FOR_UNTRUSTED_CALLER,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER, INTERFACESAFE_FOR_UNTRUSTED_CALLER);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_DATA, INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA, INTERFACESAFE_FOR_UNTRUSTED_DATA);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACE_USES_SECURITY_MANAGER, INTERFACE_USES_SECURITY_MANAGER,
        0, INTERFACE_USES_SECURITY_MANAGER /* msxml3 SP8+ */);

    /* set INTERFACE_USES_DISPEX  */

    test_IObjectSafety_set(safety, S_OK, E_FAIL /* msxml3 SP8+ */,
        INTERFACE_USES_DISPEX, INTERFACE_USES_DISPEX,
        0, 0);

    test_IObjectSafety_set(safety, S_OK, E_FAIL /* msxml3 SP8+ */,
        INTERFACE_USES_DISPEX, 0,
        0, 0);

    test_IObjectSafety_set(safety, S_OK, S_OK /* msxml3 SP8+ */,
        0, INTERFACE_USES_DISPEX,
        0, 0);

    /* set option masking */

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACE_USES_SECURITY_MANAGER,
        0,
        0);

    /* set -- inheriting previous settings */

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL,
                                                         INTERFACESAFE_FOR_UNTRUSTED_CALLER,
                                                         INTERFACESAFE_FOR_UNTRUSTED_CALLER);
    ok(hr == S_OK, "ret %08x\n", hr );
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok(hr == S_OK, "ret %08x\n", hr );
    todo_wine
    ok(broken(enabled == INTERFACESAFE_FOR_UNTRUSTED_CALLER) ||
       enabled == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACE_USES_SECURITY_MANAGER) /* msxml3 SP8+ */,
         "Expected (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACE_USES_SECURITY_MANAGER), "
         "got %08x\n", enabled);

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL,
                                                         INTERFACESAFE_FOR_UNTRUSTED_DATA,
                                                         INTERFACESAFE_FOR_UNTRUSTED_DATA);
    ok(hr == S_OK, "ret %08x\n", hr );
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok(hr == S_OK, "ret %08x\n", hr );
    todo_wine
    ok(broken(enabled == INTERFACESAFE_FOR_UNTRUSTED_DATA) ||
       enabled == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA) /* msxml3 SP8+ */,
        "Expected (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA), "
        "got %08x\n", enabled);

    IObjectSafety_Release(safety);

    IXMLDOMDocument_Release(doc);
}

static void test_XSLPattern(void)
{
    IXMLDOMDocument2 *doc;
    IXMLDOMNodeList *list;
    VARIANT_BOOL b;
    HRESULT r;
    LONG len;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void**)&doc );
    ok( r == S_OK, "CoCreateInstance(CLSID_DOMDocument) should have succeeded instead of failing with 0x%08x\n", r );
    if( r != S_OK )
        return;

    ole_check(IXMLDOMDocument2_loadXML(doc, _bstr_(szExampleXML), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* switch to XPath */
    ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern")));

    /* for XSLPattern start index is 0, for XPath it's 1 */
    ole_check(IXMLDOMDocument2_selectNodes(doc, _bstr_("root//elem[0]"), &list));
    len = 0;
    ole_check(IXMLDOMNodeList_get_length(list, &len));
    todo_wine ok(len != 0, "expected filled list\n");
    if (len)
        todo_wine expect_list_and_release(list, "E1.E2.D1");

    IXMLDOMDocument2_Release(doc);
    free_bstrs();
}

START_TEST(domdoc)
{
    IXMLDOMDocument *doc;
    HRESULT r;

    r = CoInitialize( NULL );
    ok( r == S_OK, "failed to init com\n");
    if (r != S_OK)
        return;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    if (SUCCEEDED(r))
    {
        IXMLDOMDocument_Release(doc);

        test_domdoc();
        test_persiststreaminit();
        test_domnode();
        test_refs();
        test_create();
        test_getElementsByTagName();
        test_get_text();
        test_get_childNodes();
        test_get_firstChild();
        test_removeChild();
        test_replaceChild();
        test_removeNamedItem();
        test_IXMLDOMDocument2();
        test_XPath();
        test_XSLPattern();
        test_cloneNode();
        test_xmlTypes();
        test_nodeTypeTests();
        test_DocumentSaveToDocument();
        test_DocumentSaveToFile();
        test_testTransforms();
        test_Namespaces();
        test_FormattingXML();
        test_NodeTypeValue();
        test_TransformWithLoadingLocalFile();
        test_put_nodeValue();
        test_document_IObjectSafety();
    }
    else
        win_skip("IXMLDOMDocument is not available (0x%08x)\n", r);

    test_XMLHTTP();

    CoUninitialize();
}
