/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    hashgen.cpp

Abstract:

    Table Generator for hhead.cxx, which contains all known HTTP headers for wininet project.
        This is also the location where all known HTTP headers must be added.

Author:

    Arthur Bierer (arthurbi) 12-Jan-1998

Revision History:

--*/

//
// Instructions for adding new HTTP header:
// 1. Update wininet.w and rebuild wininet.h with new HTTP_QUERY_ code
// 2. Add/Edit header to this file/program, hashgen.cpp with the 
//     new header string (see Items[] array below)
// 3. Compile new hashgen.exe, Execute with -o, write down a good seed
//      note that this may take all night to find a good seed which
//      give a nice smaller table size. (note this can be skipped if
//      you just need a quick table for dev purposes) 
// 4. Re-Execute hashgen.exe with -b# set with your seed to generate
//     hhead.cxx
// 5. Transfer new hhead.cxx file to wininet\http
// 6. Update const defines MAX_HEADER_HASH_SIZE and HEADER_HASH_SEED
//     from new hhead.cxx to wininet\http\headers.h
// 7. Transfer and checkin hashgen.cpp, wininet.w, headers,h, hhead.cxx
//     in their appropriate directories.
//


//
// Includes...
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <wininet.h>

//
// macros
//

#define IS_ARG(c)   ((c) == '-')
#define DIM(x)	(sizeof(x) / sizeof(x[0]))

#define ENUMDEF(x, y) ,x, #x, #y
#define OUTPUT_CODE_FILE "hhead.cxx" 
#define MAX_SIZE_HASHARRAY_TO_ATTEMPT 600
#define UNKNOWN_HASH_ENTRY 0 // character to put in array when when its not valid

//
// Items - This is the array that must be edited for Wininet to process new 
//  HTTP headers
//
//  Things to keep in mind before you add to this array
//  1. Headers are Alphatized for convience sake
//  2. All NULL entries MUST be at the end of the array
//  3. All HTTP_QUERY_* codes in wininet.h MUST have an entry even if they are not strings
//  4. Entries are as follows: 
//      header string, HTTP_QUERY_* code in wininet.h, flags used in wininet\http\query.cxx
//  5. All entries must be in lowercase.
//


struct Item
{
    char  *ptok;
    DWORD id;
    char  *pidName;
    char  *pFlagsName;
} Items[] = 
{
{ "Accept"              ENUMDEF(HTTP_QUERY_ACCEPT, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Accept-Charset"      ENUMDEF(HTTP_QUERY_ACCEPT_CHARSET, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Accept-Encoding"     ENUMDEF(HTTP_QUERY_ACCEPT_ENCODING, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Accept-Language"     ENUMDEF(HTTP_QUERY_ACCEPT_LANGUAGE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Accept-Ranges"       ENUMDEF(HTTP_QUERY_ACCEPT_RANGES, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Age"                 ENUMDEF(HTTP_QUERY_AGE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Allow"               ENUMDEF(HTTP_QUERY_ALLOW, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Authorization"       ENUMDEF(HTTP_QUERY_AUTHORIZATION, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Cache-Control"       ENUMDEF(HTTP_QUERY_CACHE_CONTROL, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Connection"          ENUMDEF(HTTP_QUERY_CONNECTION, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Content-Base"        ENUMDEF(HTTP_QUERY_CONTENT_BASE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Content-Description" ENUMDEF(HTTP_QUERY_CONTENT_DESCRIPTION, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Content-Disposition" ENUMDEF(HTTP_QUERY_CONTENT_DISPOSITION, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Content-Encoding"    ENUMDEF(HTTP_QUERY_CONTENT_ENCODING, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Content-Id"          ENUMDEF(HTTP_QUERY_CONTENT_ID, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Content-Language"    ENUMDEF(HTTP_QUERY_CONTENT_LANGUAGE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Content-Length"      ENUMDEF(HTTP_QUERY_CONTENT_LENGTH, (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_NUMBER)) },
{ "Content-Location"    ENUMDEF(HTTP_QUERY_CONTENT_LOCATION, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Content-Md5"         ENUMDEF(HTTP_QUERY_CONTENT_MD5, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Content-Range"       ENUMDEF(HTTP_QUERY_CONTENT_RANGE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Content-Transfer-Encoding" ENUMDEF(HTTP_QUERY_CONTENT_TRANSFER_ENCODING, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Content-Type"        ENUMDEF(HTTP_QUERY_CONTENT_TYPE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },                    
{ "Cookie"              ENUMDEF(HTTP_QUERY_COOKIE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Cost"                ENUMDEF(HTTP_QUERY_COST, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Date"                ENUMDEF(HTTP_QUERY_DATE, (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME)) },
{ "Derived-From"        ENUMDEF(HTTP_QUERY_DERIVED_FROM, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Etag"                ENUMDEF(HTTP_QUERY_ETAG, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Expect"              ENUMDEF(HTTP_QUERY_EXPECT, (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME)) },
{ "Expires"             ENUMDEF(HTTP_QUERY_EXPIRES, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Forwarded"           ENUMDEF(HTTP_QUERY_FORWARDED, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "From"                ENUMDEF(HTTP_QUERY_FROM, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Host"                ENUMDEF(HTTP_QUERY_HOST, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "If-Modified-Since"   ENUMDEF(HTTP_QUERY_IF_MODIFIED_SINCE, (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME)) },
{ "If-Match"            ENUMDEF(HTTP_QUERY_IF_MATCH, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "If-None-Match"       ENUMDEF(HTTP_QUERY_IF_NONE_MATCH, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "If-Range"            ENUMDEF(HTTP_QUERY_IF_RANGE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "If-Unmodified-Since" ENUMDEF(HTTP_QUERY_IF_UNMODIFIED_SINCE, (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME)) },
{ "Last-Modified"       ENUMDEF(HTTP_QUERY_LAST_MODIFIED, (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME)) },
{ "Link"                ENUMDEF(HTTP_QUERY_LINK, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Location"            ENUMDEF(HTTP_QUERY_LOCATION, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Mime-Version"        ENUMDEF(HTTP_QUERY_MIME_VERSION, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Max-Forwards"        ENUMDEF(HTTP_QUERY_MAX_FORWARDS, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Message-id"          ENUMDEF(HTTP_QUERY_MESSAGE_ID, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Ms-Echo-Request"     ENUMDEF(HTTP_QUERY_ECHO_REQUEST, 0) },
{ "Ms-Echo-Reply"       ENUMDEF(HTTP_QUERY_ECHO_REPLY, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Orig-Uri"            ENUMDEF(HTTP_QUERY_ORIG_URI, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Pragma"              ENUMDEF(HTTP_QUERY_PRAGMA, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Proxy-Authenticate"  ENUMDEF(HTTP_QUERY_PROXY_AUTHENTICATE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Proxy-Authorization" ENUMDEF(HTTP_QUERY_PROXY_AUTHORIZATION, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Proxy-Connection"    ENUMDEF(HTTP_QUERY_PROXY_CONNECTION, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Public"              ENUMDEF(HTTP_QUERY_PUBLIC, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Range"               ENUMDEF(HTTP_QUERY_RANGE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Referer"             ENUMDEF(HTTP_QUERY_REFERER, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Refresh"             ENUMDEF(HTTP_QUERY_REFRESH, 0) },
{ "Retry-After"         ENUMDEF(HTTP_QUERY_RETRY_AFTER, (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME)) },
{ "Server"              ENUMDEF(HTTP_QUERY_SERVER, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Set-Cookie"          ENUMDEF(HTTP_QUERY_SET_COOKIE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Title"               ENUMDEF(HTTP_QUERY_TITLE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Transfer-Encoding"   ENUMDEF(HTTP_QUERY_TRANSFER_ENCODING, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Unless-Modified-Since" ENUMDEF(HTTP_QUERY_UNLESS_MODIFIED_SINCE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Upgrade"             ENUMDEF(HTTP_QUERY_UPGRADE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Uri"                 ENUMDEF(HTTP_QUERY_URI, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "User-Agent"          ENUMDEF(HTTP_QUERY_USER_AGENT, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Vary"                ENUMDEF(HTTP_QUERY_VARY, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Via"                 ENUMDEF(HTTP_QUERY_VIA, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "Warning"             ENUMDEF(HTTP_QUERY_WARNING, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{ "WWW-Authenticate"    ENUMDEF(HTTP_QUERY_WWW_AUTHENTICATE, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
// NULL strs must be in end of array
{  NULL                 ENUMDEF(HTTP_QUERY_VERSION, HTTP_QUERY_FLAG_REQUEST_HEADERS) },         
{  NULL                 ENUMDEF(HTTP_QUERY_STATUS_CODE, HTTP_QUERY_FLAG_NUMBER) },
{  NULL                 ENUMDEF(HTTP_QUERY_STATUS_TEXT, 0) },
{  NULL                 ENUMDEF(HTTP_QUERY_RAW_HEADERS, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{  NULL                 ENUMDEF(HTTP_QUERY_RAW_HEADERS_CRLF, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{  NULL                 ENUMDEF(HTTP_QUERY_REQUEST_METHOD, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{  NULL                 ENUMDEF(HTTP_QUERY_ECHO_HEADERS, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
{  NULL                 ENUMDEF(HTTP_QUERY_ECHO_HEADERS_CRLF, HTTP_QUERY_FLAG_REQUEST_HEADERS) },
};


//
// Declarations of common strings used in creating output "C" file
//

char szFileHeader[] = 
{"/*++\n\n"
 "Copyright (c) 1997 Microsoft Corporation\n\n"
 "Module Name:\n\n"
 "    "
 OUTPUT_CODE_FILE
 "\n\n"
 "Abstract:\n\n"
 "    This file contains autogenerated table values of a perfect hash function\n"
 "    DO NOT, DO NOT EDIT THIS FILE, TO ADD HEADERS SEE hashgen.cpp\n"
 "    Contents:\n"
 "      GlobalKnownHeaders\n"
 "      GlobalHeaderHashs\n\n"
 "Author:\n\n"
 "   Arthur Bierer (arthurbi) 19-Dec-1997 (AND) my code generator[hashgen.exe]\n\n"
 "Revision History:\n\n"
 "--*/\n\n\n" };


char szComment1[] = {
"//\n"
"// GlobalHeaderHashs - array of precalculated hashes on case-sensetive set of known headers.\n"
"// This array must be used with the same hash function used to generate it.\n"
"// Note, all entries in this array are biased (++'ed) by 1 from HTTP_QUERY_ manifests in wininet.h.\n"
"//   0-ed entries indicate error values\n"
"//\n\n" };

char szComment2[] = {
"//\n"
"// GlobalKnownHeaders - array of HTTP request and response headers that we understand.\n"
"// This array must be in the same order as the HTTP_QUERY_ manifests in WININET.H\n"
"//\n\n" 
"#define HEADER_ENTRY(String, Flags, HashVal) String, sizeof(String) - 1, Flags, HashVal\n\n" };

char szDef1[] = {
"#ifdef HEADER_HASH_SEED\n"
"#if (HEADER_HASH_SEED != %u)\n"
"#error HEADER_HASH_SEED has not been updated in the header file, please copy this number to the header\n"
"#endif\n"
"#else\n"
"#define HEADER_HASH_SEED %u\n"
"#endif\n\n" };

char szDef2[] = {
"#ifdef MAX_HEADER_HASH_SIZE\n"
"#if (MAX_HEADER_HASH_SIZE != %u)\n"
"#error MAX_HEADER_HASH_SIZE has not been updated in the header file, please copy this number to the header\n"
"#endif\n"
"#else\n"
"#define MAX_HEADER_HASH_SIZE %u\n"
"#endif\n\n" };

char szDef3[] = {
"#ifdef HTTP_QUERY_MAX\n"
"#if (HTTP_QUERY_MAX != %u)\n"
"#error HTTP_QUERY_MAX is not the same as the value used in wininet.h, this indicates mismatched headers, see hashgen.cpp\n"
"#endif\n"
"#endif\n\n" };


char szIncludes[] = {
"#include <wininetp.h>\n"
"#include \"httpp.h\"\n\n" };


//
// Hash - function used to create table, 
//   THIS FUNCTION MUST BE THE SAME AS THE ONE USED in WININET
//

DWORD Hash(char *pszName, DWORD j, DWORD seed)
{
	DWORD hash = seed;

	while (*pszName)
	{
		hash += (hash << 5) + *pszName++;
	}
    return (j==0) ? hash : hash % j;
}

//
// CompareItems - a util function for qsort-ing by ID for table creation
//   in the output file
//

int __cdecl CompareItems (const void *elem1, const void *elem2 ) 
{
    const struct Item *pItem1, *pItem2;

    pItem1 = (struct Item *) elem1;
    pItem2 = (struct Item *) elem2;

    if ( pItem1->id < pItem2->id )    
    {
        return -1;
    }
    else if ( pItem1->id > pItem2->id )
    {
        return 1;
    }

    return 0;
}


//
// usage() - print out our usage instructions to command line
//

void usage() {
    fprintf(stderr,
           "\n"
           "usage: hashgen [-m[#]] [-b[#]] [-t[#]] [-o] [-p<path>] [-f<filename>]\n"
           "\n"
           "where: -m[#] = Max hash table size to test with, default = 600\n"
           "       -b[#] = Starting hash seed, default = 0\n"
           "       -t[#] = Threshold of table size to halt search at, default = 200\n"
           "       -o    = Enable optimal exhaustive search mode (can take 24+ hrs)\n"
           "       -p    = Path used for output generation\n"
           "       -f    = Output filename, \"hhead.cxx\" is assumed\n"
           "\n"
           "Instructions for adding new HTTP header:\n"
           "\t1. Update wininet.w and rebuild wininet.h with new HTTP_QUERY_ code\n"
           "\t2. Add/Edit this file/program, hashgen.cpp with the new header string\n"
           "\t3. Compile/Execute new hashgen.exe with -o, write down a good seed\n"
           "\t4. Re-Execute hashgen.exe with -b# set with your seed to generate\n"
           "\t    hhead.cxx\n"
           "\t5. Transfer new hhead.cxx file to wininet\\http\n"
           "\t6. Update const defines MAX_HEADER_HASH_SIZE and HEADER_HASH_SEED\n"
           "\t    from new hhead.cxx to wininet\\http\\headers.h\n"
           "\t7. Transfer and checkin hashgen.cpp, wininet.w, headers,h, hhead.cxx\n"
           );
    exit(1);
}

//
// MakeMeLower - Makes a lower case string using a static 255 byte array
//

LPSTR
MakeMeLower(
    IN LPSTR lpszMixedCaseStr
    )
{
    static CHAR szLowerCased[256];

    if ( lstrlen(lpszMixedCaseStr) > 255 ) 
    {
        fprintf(stderr, "Internal error: an HTTP header is too long\n\n");
        return szLowerCased;
    }

    lstrcpy( szLowerCased, lpszMixedCaseStr );
    CharLower(szLowerCased);

    return szLowerCased;
}
     


//
// main - where it all gets done !!!!
//

void
_CRTAPI1
main(
    int   argc,
    char * argv[]
    )
{
    DWORD nMax = MAX_SIZE_HASHARRAY_TO_ATTEMPT;
    DWORD dwBestNumber = 0, dwBestSeed = 0 /*349160*/ /*4458*//*202521*/;
    DWORD dwSearchThreshold = 200;
    BOOL bFoundOne = FALSE;
    BOOL bFindOptimalSeed = FALSE;
    LPSTR szPath = "";
    LPSTR szFileName = OUTPUT_CODE_FILE;
	DWORD i, j, k;
    DWORD dwValidStringsInArray = 0;
	DWORD *pHash = new DWORD[nMax];

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {
            case '?':
                usage();
                break;

            case 'm':
                nMax = (DWORD)atoi(++*argv);
                break;

            case 'b':
                dwBestSeed = (DWORD)atoi(++*argv);
                break;

            case 't':
                dwSearchThreshold = (DWORD)atoi(++*argv);
                break;
            
            case 'p':
                szPath = ++*argv;
                break;

            case 'f':
                szFileName = ++*argv;
                break;

            case 'o':
                bFindOptimalSeed = TRUE;
                break;
            default:
                fprintf(stderr,"error: unrecognized command line flag: '%c'\n", **argv);
                usage();
            }         
        } else {
            fprintf(stderr,"error: unrecognized command line argument: \"%s\"\n", *argv);
            usage();
        }
    }

    //
    // Let the Work begin...
    //

    dwBestNumber = nMax;

    if (bFindOptimalSeed)
    {
        printf("This will take a while, perhaps all night(consider a Ctrl-C)...\n");
    }

    for (i = 0; i < DIM(Items); i++ )
    {
        if ( Items[i].ptok )
            dwValidStringsInArray++;
    }

	for (i = dwBestSeed; i < (~0); i++)
	{
		//printf("%d,\n", i);
		for (j = dwValidStringsInArray; j < nMax; j++)
		{
            memset (pHash, UNKNOWN_HASH_ENTRY, nMax * sizeof(DWORD));
			for (k = 0; k < dwValidStringsInArray; k++)
			{
				DWORD HashNow = Hash(MakeMeLower(Items[k].ptok), j, i) /*% j(table_size), i(seed)*/;

                if ( HashNow > j )
                {
                    fprintf(stderr, "Error, Error - exceed table size, bad hash alg\n");
                    break;
                }

                if (pHash[HashNow] != UNKNOWN_HASH_ENTRY)
                    break;
                else
                {
                    pHash[HashNow] = Items[k].id+1;
                }
			}

            if ( k == dwValidStringsInArray )
            {
                //printf( "Found one with hash_size=%d, seed=%u...\n", j,i );
                bFoundOne = TRUE;
                goto found_one;
            }
		}
found_one:

        if ( bFoundOne )
        {
            if (j < dwBestNumber)
            {
                dwBestNumber = j;
                dwBestSeed = i;

                printf("Found a New One, hashtable_size=%d, seed=%u...\n", j ,i);
                
                if ( !bFindOptimalSeed && dwBestNumber < dwSearchThreshold )
                {
                    goto stop_search;
                }
            }

            bFoundOne = FALSE;
        }
	}

stop_search:

    if ( dwBestNumber < nMax && dwBestNumber == j)
    {
        printf("Generating %s which contains, perfect hash for known headers\n", OUTPUT_CODE_FILE);

	    FILE *f;
        CHAR szOutputFileAndPath[512];

        strcpy(szOutputFileAndPath, szPath);
        strcat(szOutputFileAndPath, szFileName);

        f = fopen(szOutputFileAndPath, "w");

        if ( f == NULL )
        {
            fprintf(stderr, "Err: Could Not Open %s for writing\n", szOutputFileAndPath);
            exit(-1);
        }

        fprintf(f, szFileHeader); // print header

        fprintf(f, szIncludes); // includes

        fprintf(f, szDef1, dwBestSeed, dwBestSeed);
        fprintf(f, szDef2, dwBestNumber, dwBestNumber);
        fprintf(f, szDef3, HTTP_QUERY_MAX);

        fprintf(f, szComment1); // print comment
         
        if ( dwBestNumber < 255 )
        {       
            fprintf(f, "const BYTE GlobalHeaderHashs[MAX_HEADER_HASH_SIZE] = {\n");
        }
        else
        {
            fprintf(f, "const WORD GlobalHeaderHashs[MAX_HEADER_HASH_SIZE] = {\n");
        }
        
        DWORD col = 0;

        //
        // spit our Nicely calculated perfect hash table..         
        //

        for ( i = 0; i < dwBestNumber; i++ )
        {
            col++;
            if ( col == 1 )
            {
                fprintf(f, "    ");
            }

            fprintf(f, "%3u, ", (BYTE) pHash[i]);    

            if ( col == 6 )
            {
                fprintf(f, "\n");
                col = 0;
            }
        }

        fprintf(f, "\n   };\n\n");


        //
        // Now spit our KnownHeader array...
        //
            
        qsort(Items, DIM(Items), sizeof(Items[0]), CompareItems);

        fprintf(f, szComment2);

        if ( DIM(Items) != (HTTP_QUERY_MAX+1) )
        {
            fprintf(stderr, "ERROR, HTTP_QUERY_MAX the wrong size,( different wininet.h's? )\n");
            return;
        }

        fprintf(f, "const struct KnownHeaderType GlobalKnownHeaders[HTTP_QUERY_MAX+1] = {\n");

	    for (j = 0; j < DIM(Items); j++)
	    {
            char szBuffer[256];
            DWORD dwHash = 0;

            sprintf(szBuffer, "    HEADER_ENTRY(\"%s\",", (Items[j].ptok ? Items[j].ptok : "\0"));
            if ( Items[j].ptok )
            {
                dwHash = Hash(MakeMeLower(Items[j].ptok), 0, dwBestSeed);
            }

            fprintf(f, "%-45s  %s, 0x%X),\n", szBuffer, Items[j].pFlagsName, dwHash);                                                 
	    }

        fprintf(f,"    };\n\n\n");

    	fclose(f);
    }
    else
    {
        fprintf(stderr, "Error, could not find an ideal number\n");
    }

}

