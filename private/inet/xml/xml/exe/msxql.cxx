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
#include "xql/query/childrenquery.hxx"
#endif

#ifndef _XQL_QUERY_CONDITION
#include "xql/query/condition.hxx"
#endif

#ifndef _XQL_PARSE_XQLPARSER
#include "xql/parser/xqlparser.hxx"
#endif

extern void LoadXMLDocument(BSTR b, IXMLDocument2 ** pp);

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
        ENDTRY
        
    }

	~xqltest()
	{
	}

    void parseXQLArgs(AString * args)
    {
    	int i = 0;
    	if (args->length() < 2)
    	{
    		printXQLHelp();
    	}
    	TRY
    	{
    	    // NOTE we skip 1st arg, which is the flag used by msxml.exe
    	    // to signal an xql request
    		for (i = 1; i < args->length(); i++)
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
    					printXQLHelp();
    					break;
    				}
    				break;
    			case '?':
    			default:
    				printXQLHelp();
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
    			printXQLHelp();
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
    		printXQLHelp();
    	}
        ENDTRY
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
            szFileName = (char *)(char *)AsciiText(filename);

            // hack for Win95, since there is no Unicode GetFullPathName.
            GetFullPathNameA(szFileName, MAX_PATH, buf, NULL);
            MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, MAX_PATH);
            
            bstrURL = wbuf;

            //checkhr(CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDocument2, (void**) ppDoc));
            LoadXMLDocument(bstrURL, ppDoc);

            //checkhr((*ppDoc)->put_caseInsensitive(false));

            //checkhr((*ppDoc)->put_URL(bstrURL));
        }

        CATCH
        {
            bstrURL = (WCHAR *) null;
            Exception::throwAgain();
        }
        ENDTRY
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
        ENDTRY

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
        ENDTRY

    }


	void printXQLHelp()
	{
		sysOut->println(_T("XQL Arguments:"));
		sysOut->println(_T("\t-xql [required]"));
		sysOut->println(_T("\t-x <XML source file name>"));
		sysOut->println(_T("\t-q <XQL query file name (as XML)>"));
        sysOut->println(_T("\t-c <result file name>"));
        sysOut->println(_T("\t-r <result file directory>"));
		sysOut->println(_T("\t-? help"));
		sysOut->println(_T("Example usage: -xql -x Select1.xml -q Select1.xql -c Select1.qcx"));
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
        case Element::DOCTYPE:
            o->print(_T("DOCTYPE"));
            dumpAttributes = true;
            dumpTagName = false;            
            break;
        case Element::ELEMENT:
            o->print(_T("ELEMENT"));
            dumpAttributes = true;
            break;
        case Element::ENTITY:
            o->print(e->getTagName());
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
            if (e->getTagName()->toString()->equalsIgnoreCase(_T("xml")))
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
            NameDef * n = e->getNameDef();
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
                o->print(a->getNameDef()->toString());
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


void xqlMain(AString * args)
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
        xqltest::GetXMLDocFromFile(o->_xmlFile, &docXMLSource);
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
    ENDTRY
}

// ===== end of file =======================================
