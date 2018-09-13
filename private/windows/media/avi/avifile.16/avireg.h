// we do this silly thing so we register InprocServer or InprocServer32
#ifdef WIN32
    #define S32 "32"
#else
    #define S32
#endif

// !!! Lots of these strings should be resources!
#define MAX_RC_CONSTANT		0

#ifdef RCINVOKED
STRINGTABLE MOVEABLE DISCARDABLE
END
#else
#define MAKERESOURCE(i)	    ((char *) (i))
static char * aszReg[] = {
     "Interface\\{00020020-0000-0000-C000-000000000046}", "AVIFile Interface 1.1",
     "Interface\\{00020020-0000-0000-C000-000000000046}\\ProxyStubClsid","{0002000d-0000-0000-C000-000000000046}",

     "Interface\\{00020021-0000-0000-C000-000000000046}", "AVIStream Interface",
     "Interface\\{00020021-0000-0000-C000-000000000046}\\ProxyStubClsid","{0002000d-0000-0000-C000-000000000046}",

     "Clsid\\{0002000d-0000-0000-C000-000000000046}","IAVIStream & IAVIFile Proxy",
     "Clsid\\{0002000d-0000-0000-C000-000000000046}\\InprocServer" S32,"avifile.dll",

     "Clsid\\{00020000-0000-0000-C000-000000000046}","Microsoft AVI Files",
     "Clsid\\{00020000-0000-0000-C000-000000000046}\\InprocServer" S32,"avifile.dll",
     "Clsid\\{00020000-0000-0000-C000-000000000046}\\AVIFile", "7",

     "Clsid\\{00020003-0000-0000-C000-000000000046}","Microsoft Wave File",
     "Clsid\\{00020003-0000-0000-C000-000000000046}\\InprocServer" S32,"wavefile.dll",
     "Clsid\\{00020003-0000-0000-C000-000000000046}\\AVIFile", "7",

     "Clsid\\{00020001-0000-0000-C000-000000000046}","AVI Compressed Stream",
     "Clsid\\{00020001-0000-0000-C000-000000000046}\\InprocServer" S32,"avifile.dll",

#ifdef DEBUG
     "Clsid\\{00020004-0000-0000-C000-000000000046}","Nigel's lyric files",
     "Clsid\\{00020004-0000-0000-C000-000000000046}\\InprocServer" S32,"lyrfile.dll",
     "Clsid\\{00020004-0000-0000-C000-000000000046}\\AVIFile", "1",
#endif
     
     "Clsid\\{00020006-0000-0000-C000-000000000046}","DIB Sequences",
     "Clsid\\{00020006-0000-0000-C000-000000000046}\\InprocServer" S32,"dseqfile.dll",
     "Clsid\\{00020006-0000-0000-C000-000000000046}\\AVIFile", "7",

     "Clsid\\{0002000A-0000-0000-C000-000000000046}","TGA Sequences",
     "Clsid\\{0002000A-0000-0000-C000-000000000046}\\InprocServer" S32,"tgafile.dll",
     "Clsid\\{0002000A-0000-0000-C000-000000000046}\\AVIFile", "7",

     "Clsid\\{00020009-0000-0000-C000-000000000046}","Simple AVIFile unmarshaller",
     "Clsid\\{00020009-0000-0000-C000-000000000046}\\InprocServer" S32,"avifile.dll",

     "Clsid\\{00020007-0000-0000-C000-000000000046}","Autodesk FLx",
     "Clsid\\{00020007-0000-0000-C000-000000000046}\\InprocServer" S32,"flifile.dll",
     "Clsid\\{00020007-0000-0000-C000-000000000046}\\AVIFile", "1",

#ifdef DEBUG
     "Clsid\\{0002000E-0000-0000-C000-000000000046}","Various Medbits Formats",
     "Clsid\\{0002000E-0000-0000-C000-000000000046}\\InprocServer" S32,"mbitfile.dll",
     "Clsid\\{0002000E-0000-0000-C000-000000000046}\\AVIFile", "1",

     "Clsid\\{00020008-0000-0000-C000-000000000046}","QuickTime Movies",
     "Clsid\\{00020008-0000-0000-C000-000000000046}\\InprocServer" S32,"qtfile.dll",
     "Clsid\\{00020008-0000-0000-C000-000000000046}\\AVIFile", "1",

     "Clsid\\{5C2B8200-E2C8-1068-B1CA-6066188C6002}","JPEG (JFIF) Files",
     "Clsid\\{5C2B8200-E2C8-1068-B1CA-6066188C6002}\\InprocServer" S32,"jfiffile.dll",
     "Clsid\\{5C2B8200-E2C8-1068-B1CA-6066188C6002}\\AVIFile", "3",
#endif
     
     "Clsid\\{0002000F-0000-0000-C000-000000000046}","ACM Compressed Audio Stream",
     "Clsid\\{0002000F-0000-0000-C000-000000000046}\\InprocServer" S32,"acmcmprs.dll",

     "AVIFile","Video for Windows 1.1 Information",
     "AVIFile\\RIFFHandlers\\AVI","{00020000-0000-0000-C000-000000000046}",
     "AVIFile\\RIFFHandlers\\WAVE","{00020003-0000-0000-C000-000000000046}",

     "AVIFile\\Extensions\\AVI","{00020000-0000-0000-C000-000000000046}",
     "AVIFile\\Extensions\\WAV","{00020003-0000-0000-C000-000000000046}",
     "AVIFile\\Extensions\\DIB","{00020006-0000-0000-C000-000000000046}",
     "AVIFile\\Extensions\\BMP","{00020006-0000-0000-C000-000000000046}",
     "AVIFile\\Extensions\\FLI","{00020007-0000-0000-C000-000000000046}",
     "AVIFile\\Extensions\\FLC","{00020007-0000-0000-C000-000000000046}",
     "AVIFile\\Extensions\\TGA","{0002000A-0000-0000-C000-000000000046}",
#ifdef DEBUG
     "AVIFile\\Extensions\\LYR","{00020004-0000-0000-C000-000000000046}",
     "AVIFile\\Extensions\\GIF","{0002000E-0000-0000-C000-000000000046}",
     "AVIFile\\Extensions\\PCX","{0002000E-0000-0000-C000-000000000046}",
     "AVIFile\\Extensions\\MOV","{00020008-0000-0000-C000-000000000046}",
     "AVIFile\\Extensions\\JPG","{5C2B8200-E2C8-1068-B1CA-6066188C6002}",
#endif
     "AVIFile\\Compressors\\vids","{00020001-0000-0000-C000-000000000046}",
     "AVIFile\\Compressors\\auds","{0002000F-0000-0000-C000-000000000046}",

     NULL, NULL};

#endif
