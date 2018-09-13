#include <windows.h>
#include <stdio.h>


char achLine[17];

void Dump( PBYTE pb, DWORD cb ) {
    DWORD i, j;


    for( i = 0; i < cb; i += 16 ) {
        printf( "%04X  ", (int)i );
        FillMemory( achLine, sizeof(achLine), 0);

        for( j = 0; j < 16; j++ ) {
            BYTE b;

            if (i+j >= cb) {
                break;
            }
            printf( "%02X ", (b = pb[i+j]) );
            achLine[j] = b >= ' ' ? b : '.';
        }

        achLine[j] = '\0';
        printf(  "  #%s#\n", achLine );

    }

}

TCHAR gszName[MAX_PATH];

LPTSTR MakeCFFmtName( UINT i ) {

    switch( i ) {
    case CF_TEXT:           lstrcpy( gszName, "CF_TEXT");              break;
    case CF_BITMAP:         lstrcpy( gszName, "CF_BITMAP");            break;
    case CF_METAFILEPICT:   lstrcpy( gszName, "CF_METAFILEPICT");      break;
    case CF_SYLK:           lstrcpy( gszName, "CF_SYLK");              break;
    case CF_DIF:            lstrcpy( gszName, "CF_DIF");               break;
    case CF_TIFF:           lstrcpy( gszName, "CF_TIFF");              break;
    case CF_OEMTEXT:        lstrcpy( gszName, "CF_OEMTEXT");           break;
    case CF_DIB:            lstrcpy( gszName, "CF_DIB");               break;
    case CF_PALETTE:        lstrcpy( gszName, "CF_PALETTE");           break;
    case CF_PENDATA:        lstrcpy( gszName, "CF_PENDATA");           break;
    case CF_RIFF:           lstrcpy( gszName, "CF_RIFF");              break;
    case CF_WAVE:           lstrcpy( gszName, "CF_WAVE");              break;
    case CF_UNICODETEXT:    lstrcpy( gszName, "CF_UNICODETEXT");       break;
    case CF_ENHMETAFILE:    lstrcpy( gszName, "CF_ENHMETAFILE");       break;
    case CF_HDROP:          lstrcpy( gszName, "CF_HDROP");             break;
    case CF_LOCALE:         lstrcpy( gszName, "CF_LOCALE");            break;
    case CF_MAX:            lstrcpy( gszName, "CF_MAX");               break;
    case CF_OWNERDISPLAY:   lstrcpy( gszName, "CF_OWNERDISPLAY");      break;
    case CF_DSPTEXT:        lstrcpy( gszName, "CF_DSPTEXT");           break;
    case CF_DSPBITMAP:      lstrcpy( gszName, "CF_DSPBITMAP");         break;
    case CF_DSPMETAFILEPICT:lstrcpy( gszName, "CF_DSPMETAFILEPICT");   break;
    case CF_DSPENHMETAFILE: lstrcpy( gszName, "CF_DSPENHMETAFILE");    break;

    default:
        gszName[0] = TEXT('\0');
        GetClipboardFormatName(i, gszName, sizeof(gszName) / sizeof(TCHAR));
        break;
    }

    return gszName;
}

void
__cdecl
main(
    int cArgs,
    char **szArg
    )
{
    UINT iCF = 0;
    UINT iRet = 0;
    LPTSTR szName;

    if( cArgs > 1 && (
            (*szArg[1] == '-' || *szArg[1] == '/') &&
            (szArg[1][1] == '?' || lstrcmpi(&(szArg[1][1]), "help") == 0) ) ) {
        fprintf( stderr, "usage: dumpclip [\"clipboard format\"]\n" );
        ExitProcess((DWORD)-1);
    }


    OpenClipboard(NULL);

    for( iCF = EnumClipboardFormats( 0 ); iCF != 0; iCF = EnumClipboardFormats( iCF ) ) {
        HANDLE hClipData;
        PVOID pData;

        szName = MakeCFFmtName(iCF);

        if (cArgs == 1 || lstrcmpi(szArg[1], szName) == 0 ) {

            hClipData = GetClipboardData(iCF);

            pData = GlobalLock(hClipData);

            printf( "\n%s format:\n", szName );
            Dump( pData, (DWORD)GlobalSize(hClipData) );

            GlobalUnlock(hClipData);
            printf( "\n" );
        }
    }

    CloseClipboard();

    ExitProcess(iRet);
}
