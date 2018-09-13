/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/

#include <windows.h>
#include <objbase.h>
#include <xmlparser.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "FileStream.hxx"
#include "MyFactory.hxx"

//----------------------------------------------------------------------------------
void printParserError(HRESULT hr, IXMLParser* xp)
{
    printf("\n#### ");
    BSTR reason = NULL;
    xp->GetErrorInfo(&reason);
    if (reason != NULL)
    {
        printf("%S",reason);
        ::SysFreeString(reason);
    }
    else
    {
        printf("Unknown Error: %lx\n", hr);
    }

    long line;
    line = xp->GetLineNumber();
    printf("Line %.4ld: ", line);

    const TCHAR* buf;
    ULONG len;
	ULONG pos;
    xp->GetLineBuffer(&buf, &len, &pos);
    if (buf != NULL)
    {
        pos = xp->GetLinePosition();
        ULONG i = 0;
        if (pos > 60) {
            i = pos - 60;
            printf("...");
        }
        for (;  i < len && buf[i] != '\n' && buf[i] != 0; i++)
        {
            if (buf[i] == '\t')
                putchar(' ');
            else
                putchar(buf[i]);
        }
        printf("\nPos  %.4ld: ", pos);
        i = 0;
        if (pos > 60)
        {
            printf("...");
            i = pos - 60;
        }
        for (; i < pos-1; i++)            
        {
            putchar('-');
        }
        putchar('^');
    }

    printf("\n");
}

//----------------------------------------------------------------------------------
int __cdecl main(int argc, char* argv[])
{
	HRESULT hr = CoInitialize(0);

	char* name = NULL;
    bool compact = false;

    if (argc == 1)
    {
        printf("Usage: %s [-c] filename\n", argv[0]);
        printf("Pretty prints the XML in the given file to standard output.\n");
        printf("The -c option causes compact output.\n");
        return 1;
    }

    char* arg = argv[1];
    if ((arg[0] == '-') && (arg[1] == 'c'))
    {
        compact = true;
        arg = argv[2];
    }

    FileStream* in = new FileStream;
    if (! in->open(arg))
    {
        printf("Error opening input file: %s\n", name);
        return 1;
    }

    IXMLParser* xp = NULL;

    hr = CoCreateInstance(CLSID_XMLParser, NULL, CLSCTX_INPROC_SERVER, 
                                IID_IXMLParser, (void**)&xp);

    if (FAILED(hr))
    {
        printf("Failed to co-create the xml parser, hr=%lx\n", (long)hr);
        return hr;
    }


    xp->SetInput(in);

    MyFactory* factory = new MyFactory(compact);
    xp->SetFactory(factory);
    factory->Release();

    xp->SetFlags( XMLFLAG_CASEINSENSITIVE | XMLFLAG_NOWHITESPACE ) ;

    hr = xp->Run(-1);
    if (hr != 0)
    {
        printParserError(hr, xp);
    }

    xp->Release();

	return 0;	
}


