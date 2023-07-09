
/*****************************************************************************

                        D D E   C A L L B A C K

    Name:       callback.h
    Date:       21-Jan-1994
    Creator:    John Fu

    Description:
        This is the header file for callback.c

*****************************************************************************/



HDDEDATA EXPENTRY DdeCallback(
    WORD        wType,
    WORD        wFmt,
    HCONV       hConv,
    HSZ         hszTopic,
    HSZ         hszItem,
    HDDEDATA    hData,
    DWORD       lData1,
    DWORD       lData2);


DWORD GetClipsrvVersion(
    HWND    hwndChild);
