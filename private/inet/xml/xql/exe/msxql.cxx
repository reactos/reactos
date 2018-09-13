/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#ifndef _BSTR_HXX
#include "core\com\bstr.hxx"
#endif

#ifndef _VARIANT_HXX
#include "core/com/variant.hxx"
#endif

#ifndef _CORE_IO_FILE
#include "core\io\File.hxx"
#endif

#ifndef _XML_OM_DOCUMENT
#include "xml\om\Document.hxx"
#endif

#ifndef _XQL_QUER_QUERYENUMERATION
#include "xql\query\ChildrenQuery.hxx"
#endif

#ifndef _XQL_QUERY_STRINGOPERAND
#include "xql\query\StringOperand.hxx"
#endif

#ifndef _XQL_QUERY_CONDITION
#include "xql\query\Condition.hxx"
#endif

#ifndef _XQL_PARSE_XQLPARSER
#include "xql\parser\XQLParser.hxx"
#endif

#if DBG==1
DWORD g_dwFALSE;
#endif

void dumpTree(Element * e, PrintStream * o, String * indent);

SRPrintStream sysOut = {null};
SRPrintStream sysErr = {null};

static int g_cOutputFiles = 0;

class xqltest : public Base
{
	DECLARE_CLASS_MEMBERS(xqltest, Base);

	RString _xmlFile;           // XML source file that we will query against
	RString _xqlFile;           // XQL query written in our internal XML grammar
	RString _resultFile;        // file containing nodes in the XQL query result
	RString _resultDirectory;   // directory where result file will be placed
	RString _fileNameRoot;      // default root of file names

    RDocument source;

    void testXQL(Element * docToQuery)        
    {
        _reference<IXMLDocument2> doc;
        _reference<IXMLElement2> root;
        Query * qy;

        TRY
        {
            XQLParser * xql = new XQLParser();

            Enumeration * en;
            Element * e;
            Element * eNext;
            HANDLE hNext;

            GetXMLDocFromFile(_xqlFile, &doc);

            checkhr(doc->get_root(&root));

            checkhr(root->QueryInterface(__uuidof(Element), (void **) &e));

            eNext = e->getFirstChild(&hNext);
            while(eNext)
            {
                String * q = eNext->getText();
                qy = xql->parse(q);
                qy->setContext(null, docToQuery);

                DumpXQLResultToFile(qy);
                eNext = e->getNextChild(&hNext);
            }
        }
        CATCH
        {
            Exception * e = GETEXCEPTION();
            PrintStream* stdout = StdIO::getOut();
            stdout->println(e->getMessage());
            stdout->println(_T("Caught exception during XQL parse: "));
            stdout->println(e->toString());
            stdout->println(_T("\n"));
        }
        
    }

    static void main(AString * args)
    {
        _reference<IXMLDocument2> docXMLSource;
        _reference<IXMLElement2>    eXMLSource;
        Element *         eDoc = null;          // topmost element in doc

        TRY
        {            
            sysOut = StdIO::getOut();
            sysErr = StdIO::getErr();
		    xqltest * o = new xqltest();

            o->parseXQLArgs(args);
            GetXMLDocFromFile(o->_xmlFile, &docXMLSource);
            docXMLSource->get_root(&eXMLSource);
            eXMLSource->QueryInterface(__uuidof(Element), (void**)&eDoc);

            // test xql query on doc
            o->testXQL(eDoc);
        }
        CATCH
        {
            Exception * e = GETEXCEPTION();
            PrintStream* stdout = StdIO::getOut();
            stdout->println(e->getMessage());
            stdout->println(_T("Caught exception during load: "));
            stdout->println(e->toString());
            stdout->println(_T("\n"));
        }
	}

	~xqltest()
	{
	}

    void parseXQLArgs(AString * args)
    {
    	int i = 0;
    	if (args->length() < 2)
    	{
    		printHelp();
    	}
    	TRY
    	{
    		for (i = 0; i < args->length(); i++)
    		{
    			switch((*args)[i]->charAt(0))
    			{
    			case '-':
    			case '/':
    				switch(Character::toLowerCase((*args)[i]->charAt(1)))
    				{
    				case 'x':
    					_xmlFile = (*args)[++i];
    					break;
    				case 'q':
    					_xqlFile = (*args)[++i];
    					break;
    				case 'c':
    					_resultFile = (*args)[++i];
    					break;
    				case 'r':
    					_resultDirectory = (*args)[++i];
    					break;
    				default:
    					printHelp();
    					break;
    				}
    				break;
    			case '?':
    			default:
    				printHelp();
    				break;
    			}
    		}

            // get filename root from first non-null arg
    		if (_xmlFile)
    		{
    		    _fileNameRoot = getFileNameRoot(_xmlFile);
    	    }
    		else if (_xqlFile)
    		{
    		    _fileNameRoot = getFileNameRoot(_xqlFile);
    	    }
    		else if (_resultFile)
    		{
    		    _fileNameRoot = getFileNameRoot(_resultFile);
    	    }
    	    else
    	    {
    	        // no args: print help and bail out
    			printHelp();
    			return;
    	    }


            Assert(_fileNameRoot != null);
    		if (!_xmlFile)
    		{
    		    _xmlFile = concatenate(_fileNameRoot, _T(".xml"));
    	    }

    		if (!_xqlFile)
    		{
    		    _xqlFile = concatenate(_fileNameRoot, _T(".xql"));
    	    }

    		if (!_resultFile)
    		{
    		    _resultFile = concatenate(_fileNameRoot, _T(".qcx"));
    	    }

    		if (_resultDirectory != null)
    		{
    		    _resultFile = String::add(concatenate(_resultDirectory, _T("\\")), _resultFile, null);
    	    }

    	}
    	CATCHE
    	{
    		sysOut->print(_T("*** Error processing arg "));
    		sysOut->println((*args)[i]);
    		printHelp();
    	}
    }


    static
    void    
    GetXMLDocFromFile(String * filename, IXMLDocument2 ** ppDoc)
    {
        char *          szFileName = NULL;
        char            buf[MAX_PATH];
        WCHAR           wbuf[MAX_PATH];
        bstr            bstrURL;

        TRY
        {
            //
            // BUGBUG - holding on to this string is unsafe.  The gc could delete it from under us!
            //
            szFileName = (char *)String::toCharZA(filename);

            // hack for Win95, since there is no Unicode GetFullPathName.
            GetFullPathNameA(szFileName, MAX_PATH, buf, NULL);
            MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, MAX_PATH);
            
            bstrURL = wbuf;

            checkhr(CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDocument2, (void**) ppDoc));

            checkhr((*ppDoc)->put_caseInsensitive(false));

            checkhr((*ppDoc)->put_URL(bstrURL));
        }

        CATCH
        {
            bstrURL = (WCHAR *) null;
            Exception::throwAgain();
        }
    }

    void
    DumpXQLResultToFile(Query * qy)
    {

		TRY
		{
		    FileOutputStream * out;
		    PrintStream * ps;
		    Element * e;

            out = FileOutputStream::newFileOutputStream(_resultFile, false);
            ps = PrintStream::newPrintStream(out, true);

            // run the query by iterating through its output
            while(qy->hasMoreElements())
            {
                e = (Element *) qy->nextElement();
                dumpTree(e, ps, String::emptyString());
            }

			out->close();
		}
		CATCH 
		{
			sysErr->print(_T("Error writing result file"));
			sysErr->println(GETEXCEPTION());
		}

    }

    void
    DumpXMLTreeToFile(Element * e)
    {

        if (e == null)
        {
            return;
        }

        _resultFile = checkFilenameAppendSuffix(_resultFile, _T(".qcx"), g_cOutputFiles++);

		TRY
		{
		    FileOutputStream * out;

            out = FileOutputStream::newFileOutputStream(_resultFile, false);
            dumpTree(e, PrintStream::newPrintStream(out, true), String::emptyString());
			out->close();
		}
		CATCH 
		{
			sysErr->print(_T("Error writing result file"));
			sysErr->println(GETEXCEPTION());
		}

    }


	void printHelp()
	{
		sysOut->println(_T("Arguments:"));
		sysOut->println(_T("\t-x <XML source file name>"));
		sysOut->println(_T("\t-q <XQL query file name (as XML)>"));
        sysOut->println(_T("\t-c <output context file name>"));
		sysOut->println(_T("\t-? help"));
		exit(0);
	}


    String * getFileNameRoot(String * fileName)
    {
        return fileName->substring(0, fileName->lastIndexOf(_T('.')));
    }


    String * concatenate(String * str1, TCHAR * str2)
    {
        return String::add(str1, String::newString(str2), null);
    }


    String * checkFilename(String * in, TCHAR * extension)
    {
        String * out;

        Assert(_xmlFile != null);

        if (_fileNameRoot == null)
        {
            int ichDot = _xmlFile->lastIndexOf(_T('.'));
            _fileNameRoot = _xmlFile->substring(0, ichDot);
        }

        if (in == null)
        {
            int i = _xmlFile->lastIndexOf(_T('.'));
            in = (i == -1) ? _xmlFile : _xmlFile->substring(0, i);
        }

        if (in->lastIndexOf(_T('.')) == -1)
        {
            out = String::add(in, String::newString(extension), null);
        }
        else
        {
            out = in;
        }

        return out;
    }

    String * checkFilenameAppendSuffix(String * in, TCHAR * extension, int iSuffix = 0)
    {
        String * out;

        if (_fileNameRoot == null)
        {
            int ichDot = _xmlFile->lastIndexOf(_T('.'));
            _fileNameRoot = _xmlFile->substring(0, ichDot);
        }

        if (in == null)
        {
            int i = _xmlFile->lastIndexOf(_T('.'));
            in = (i == -1) ? _xmlFile : _xmlFile->substring(0, i);
        }

        if (in->lastIndexOf(_T('.')) == -1)
        {
            out = String::add(in, String::newString(extension), null);
        }
        else
        {
            if (iSuffix == 0)
            {
                out = in;
            }
            else
            {
                out = String::add(String::add(_fileNameRoot, String::newString(iSuffix), null), String::newString(extension), null);
            }
        }

        return out;
    }
};

DEFINE_CLASS_MEMBERS(xqltest, _T("xqltest"), Base);

// fake instance for _root.cxx
HINSTANCE g_hInstance = NULL;

extern TAG tagPointerCache;

void __cdecl 
CPlusMain(int argc, char * argv[])
{
	STACK_ENTRY;

    sysOut = StdIO::getOut();
    sysErr = StdIO::getOut();
	TRY
	{
		AString * args = new (argc - 1) AString;
		for (int i = 1; i < argc; i++)
		{
			(*args)[i - 1] = String::newString(argv[i]);
		}
		// hardcoded for now
    	xqltest::main(args);
	}
	CATCH
	{
        StdIO::getOut()->println(String::add(String::newString("Exception: "), GETEXCEPTION()->toString(), null));
	}
    sysOut = null;
    sysErr = null;
    Base::checkZeroCountList();
}

#if 0

STDAPI
WndProcMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
   case WM_DESTROY:
       PostQuitMessage(0);
       break;

   default:
       return DefWindowProc(hwnd, msg, wParam, lParam);
   }

   return 0;
}

HINSTANCE			g_hInst;
HWND               g_hwndMain;

EXTERN_C int PASCAL
WinMain(
        HINSTANCE hinst,
        HINSTANCE hPrevInst,
        LPSTR szCmdLine,
        int nCmdShow)
{
	g_hInst = hinst;
	CoInitialize(NULL);
    // Create "main" window.  This window is used to keep Windows
    // and OLE happy while we are waiting for something to happen
    // when launched to handle an embedding.

    WNDCLASS            wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProcMain;   // windows of this class.
    wc.hInstance = g_hInst;
    wc.hIcon = NULL;
    wc.lpszClassName = "Main";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    if (!RegisterClass(&wc))
    {
        goto Cleanup;
    }

    g_hwndMain = CreateWindow(
            "Main",
            NULL, WS_OVERLAPPEDWINDOW,
            0, 0, 0, 0, NULL, NULL, g_hInst, NULL);
    if (!g_hwndMain)
    {
        goto Cleanup;
    }

	CPlusMain(__argc, __argv);
	CoUninitialize();

Cleanup:

	return 0;
}

#else

int __cdecl main(int argc, char ** argv)
{
	CoInitialize(NULL);
	CPlusMain(argc, argv);
//	CoUninitialize();
	return 0;
}

extern void MTInit();
extern void MTExit();

EXTERN_C void 
Runtime_init()
{
	EnableTag(11, FALSE);	// !SYMBOLS
	EnableTag(3, FALSE);	// tagAssertExit

	g_dwTlsIndex = GetTlsIndex();
	MTInit();

    EnumWrapper::classInit();
    File::classInit();
    XQLParser::classInit();
}

extern void ClearReferences();

EXTERN_C void
Runtime_exit()
{
    // free global references
    ClearReferences();

    MTExit();
}

#endif


    // dumpTree dumps a tree out in the following format:
    // 
    // root
    // |---node
    // |---node2
    // |   |---foo
    // |   +---bar.
    // +---lastnode
    //     |---apple
    //     +---orange.
    //

    void dumpTree(Element * e, PrintStream * o, String * indent) //throws IOException
    {
        Element * eChild;
        HANDLE hChild;

        if (indent->length() > 0)
        {
            o->print(indent);
            o->print(_T("---"));
        }
        // Once we've printed the '+', from then on we are
        // to print a blank space, since the '+' means we've
        // reached the end of that branch.
        String * lines = indent->replace('+',' ');
        bool dumpText = false;
        bool dumpTagName = true;
        bool dumpAttributes = false;

        switch (e->getType()) {
        case Element::CDATA:
            o->print(_T("CDATA"));
            dumpText = true;
            break;
        case Element::COMMENT:
            o->print(_T("COMMENT"));
            dumpTagName = false;
            break;
        case Element::DOCUMENT:
            o->print(_T("DOCUMENT"));
            break;
        case Element::_DTD:
            o->print(_T("DOCTYPE"));
            dumpAttributes = true;
            dumpTagName = false;            
            break;
        case Element::ELEMENT:
            o->print(_T("ELEMENT"));
            dumpAttributes = true;
            break;
        case Element::ENTITY:
            o->print(e->getTagName()->getName());
            dumpTagName = false;
            if (e->numElements() == 0) dumpText = true;
            break;
        case Element::ENTITYREF:
            o->print(_T("ENTITYREF"));
            dumpText = true;
            break;
        case Element::NOTATION:
            o->print(_T("NOTATION"));
            dumpText = true;
            break;
        case Element::ELEMENTDECL:
            o->print(_T("ELEMENTDECL"));
            break;
        case Element::PCDATA:
            o->print(_T("PCDATA"));
            dumpText = true;
            break;
        case Element::PI:
            if (e->getTagName()->getName()->toString()->equalsIgnoreCase(_T("xml")))
            {
                o->print(_T("XMLDECL"));
                dumpAttributes = true;
                dumpTagName = false;
            } else {
                o->print(_T("PI"));
                dumpTagName = true;
            }
            break;
        }
            
        if (dumpTagName) {
            Name * n = e->getTagName();
            if (n != null) 
            {
                o->print(_T(" "));
                o->print(n->toString());
            }
        }
        if (e->getType() == Element::ENTITY) 
        {
            Entity * en = (Entity *)e;
            o->print(_T(" "));
            o->print(en->getName());
        } 
        else if (e->getType() == Element::ELEMENTDECL)
        {
            ElementDecl * ed = (ElementDecl *)e;
            o->print(_T(" "));

        }
        if (dumpAttributes)
        {
            Element * a;
            HANDLE h;
            a = e->getFirstAttribute(&h);
            while (a)
            {
                Element * aNext;
                o->println(String::emptyString());
                o->print(lines);
                aNext = e->getNextAttribute(&h);
                if (! aNext)
                {
                    o->print(_T("   |---"));
                }
                else
                {
                    o->print(_T("   +---"));
                }
                o->print(_T("ATTRIBUTE "));
                o->print(a->getTagName()->toString());
                o->print(_T(" \""));
                o->print(a->getValue()->toString());
                o->print(_T("\""));

                a = aNext;
            }
        }

        if (dumpText && e->getText() != null) 
        {
            o->print(_T(" \""));
            o->print(e->getText());
            o->print(_T("\""));
        }
        o->println(String::emptyString());

        String * newLines = String::emptyString();
        if (lines->length() > 0) 
        {
            newLines = String::add(lines, String::newString(_T("   |")), null);
        } 
        else 
        {
            newLines = String::newString(_T("|"));
        }

        eChild = e->getFirstChild(&hChild);
        while (eChild)
        {
            Element * eNext = e->getNextChild(&hChild);
            if (! eNext ) 
            {
                if (lines->length() > 0) 
                {
                    newLines = String::add(lines, String::newString(_T("   +")), null);
                } 
                else 
                {
                    newLines = String::newString(_T("+"));
                }
            }
            dumpTree(eChild,o,newLines);
            eChild = eNext;
        }
        o->flush();
    }

// ===== end of file =======================================
