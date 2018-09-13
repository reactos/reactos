/****************************************************************************

    INIT.C

    This file handles the parts related to initializing the features based
    on keynames and values in the WIN.INI file.

****************************************************************************/

#include    <string.h>
#include    <stdlib.h>
#include    "windows.h"
#include    "Dialogs.h"
#include    "skeys.h"
#include    "access.h"

/****************************************************************************

    Declaration of externs

****************************************************************************/

                                                    /* from DIALOGS.C */
extern FILTERKEYS    FilterKeysParam;
extern STICKYKEYS    StickyKeysParam;
extern MOUSEKEYS     MouseKeysParam;
extern TOGGLEKEYS    ToggleKeysParam;
extern ACCESSTIMEOUT TimeOutParam;
extern SOUNDSENTRY   SoundSentryParam;
extern SERIALKEYS    SerialKeysParam;
extern char          SK_ActivePort[MAX_PATH];

extern	BOOL IsSerialKeysInstalled();

#ifdef MYSK
extern MYSERIALKEYS  MySerialKeysParam;
#endif
extern INT           fShowSoundsOn;
// extern BOOL fSaveStickyKeys;
// extern BOOL fSaveFilterKeys;
// extern BOOL fSaveMouseKeys;
// extern BOOL fSaveToggleKeys;
// extern BOOL fSaveTimeOut;
// extern BOOL fSaveSerialKeys;
// extern BOOL fSaveSoundSentry;
// extern BOOL fSaveShowSounds;

void InitFilterKeys(HWND,HANDLE);

/* extern    int       fno_mouse;  */
                                                    /* from GIDI-INI.C */

extern    HANDLE    hInst;

/****************************************************************************

    Declaration of variables

****************************************************************************/

int OkInitMessage (HWND hWnd, WORD wnumber)

{
    int   ianswer;
    char sreadbuf[255];
    char    sbuf[45];
    {

    LoadString (hInst,wnumber,(LPSTR)sreadbuf,245);
    LoadString (hInst,IDS_TITLE,(LPSTR)sbuf,35);
    ianswer = MessageBox (hWnd, (LPSTR)sreadbuf, (LPSTR)sbuf, MB_SYSTEMMODAL | MB_YESNO|MB_ICONHAND);
    }
return(ianswer);
}


/****************************************************************************

    FUNCTION:    InitFilterKeys()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void InitFilterKeys(hWnd,hInst)
HWND hWnd;                          /* window handle */
HANDLE    hInst;                    /* current instance */
{
    BOOL fStatus;
    FILTERKEYS    *lp_FilterKeys_Param;
    lp_FilterKeys_Param = &FilterKeysParam;

    FilterKeysParam.cbSize = sizeof(FilterKeysParam);
    fStatus = SystemParametersInfo(
                  SPI_GETFILTERKEYS,
                  0,
                  lp_FilterKeys_Param,
                  0);

}

/****************************************************************************

    FUNCTION:    InitStickeyKeys()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void InitStickeyKeys()
{
    BOOL fStatus;
    STICKYKEYS        *lp_StickyKeys_Param;
    lp_StickyKeys_Param = &StickyKeysParam;

    StickyKeysParam.cbSize = sizeof(StickyKeysParam);
    fStatus = SystemParametersInfo(
                  SPI_GETSTICKYKEYS,
                  0,
                  lp_StickyKeys_Param,
                  0);

}

/****************************************************************************

    FUNCTION:    InitMouseKeys()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void InitMouseKeys()
{
    BOOL fStatus;
    MOUSEKEYS    *lp_MouseKeys_Param;

    lp_MouseKeys_Param = &MouseKeysParam;
    MouseKeysParam.cbSize = sizeof(MouseKeysParam);
    fStatus = SystemParametersInfo(
                  SPI_GETMOUSEKEYS,
                  0,
                  lp_MouseKeys_Param,
                  0);

}

/****************************************************************************

    FUNCTION:    InitToggleKeys()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void InitToggleKeys()
{
    BOOL fStatus;
    TOGGLEKEYS    *lp_ToggleKeys_Param;
    lp_ToggleKeys_Param = &ToggleKeysParam;


    ToggleKeysParam.cbSize = sizeof(ToggleKeysParam);
    fStatus = SystemParametersInfo(
                  SPI_GETTOGGLEKEYS,
                  0,
                  lp_ToggleKeys_Param,
                  0);

}

/****************************************************************************

    FUNCTION:    InitTimeOut()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void InitTimeOut()
{
    BOOL fStatus;
    ACCESSTIMEOUT        *lp_TimeOut_Param;
    lp_TimeOut_Param = &TimeOutParam;


    TimeOutParam.cbSize = sizeof(TimeOutParam);
    fStatus = SystemParametersInfo(
                  SPI_GETACCESSTIMEOUT,
                  0,
                  lp_TimeOut_Param,
                  0);

}

/****************************************************************************

    FUNCTION:    InitSerialKeys()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void InitSerialKeys()
{

#ifdef MYSK
    BOOL fStatus;

    MYSERIALKEYS          *lp_SerialKeys_Param;
    lp_SerialKeys_Param = &MySerialKeysParam;
    MySerialKeysParam.cbSize = sizeof(MySerialKeysParam);
    fStatus = SystemParametersInfo(
                  SPI_GETSERIALKEYS,
                  0,
                  lp_SerialKeys_Param,
                  0);
    // hack around -1 returned by VxD here if not turned on.
    if( MySerialKeysParam.iComName == -1 )
        MySerialKeysParam.iComName =  0;

#else

	SerialKeysParam.cbSize = sizeof(SerialKeysParam);
	SerialKeysParam.lpszActivePort = SK_ActivePort;
	SerialKeysParam.lpszPort = NULL;

	SKEY_SystemParametersInfo
	(
		SPI_GETSERIALKEYS,
                0,
		&SerialKeysParam,0
	);
#endif
}

/****************************************************************************

    FUNCTION:    InitShowSounds()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void InitShowSounds()
{
    BOOL fStatus;

    fStatus = SystemParametersInfo(
                  SPI_GETSHOWSOUNDS,
                  sizeof(fShowSoundsOn),
                  &fShowSoundsOn,
                  0);
}

/****************************************************************************

    FUNCTION:    InitSoundSentry()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void InitSoundSentry()
{
    BOOL fStatus;
    SOUNDSENTRY        *lp_SoundSentry_Param;
    lp_SoundSentry_Param = &SoundSentryParam;

    SoundSentryParam.cbSize = sizeof(SoundSentryParam);
    fStatus = SystemParametersInfo(
                  SPI_GETSOUNDSENTRY,
                  0,
                  lp_SoundSentry_Param,
                  0);

}

/****************************************************************************

    FUNCTION:    InitFeatures()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void InitFeatures(hWnd,hInst)
HWND hWnd;                        /* window handle                */
HANDLE    hInst;                    /* current instance                */
{
    InitFilterKeys(hWnd,hInst);
    InitStickeyKeys();
    InitMouseKeys();
    InitToggleKeys();
    InitTimeOut();
    InitSerialKeys();
    InitSoundSentry();
    InitShowSounds();

}

/****************************************************************************

    FUNCTION:    SaveFilterKeys()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void SaveFilterKeys()
{
    FILTERKEYS    *lp_FilterKeys_Param;
    lp_FilterKeys_Param = &FilterKeysParam;

    SystemParametersInfo(
        SPI_GETFILTERKEYS,
        0,
        lp_FilterKeys_Param,
        0);

    SystemParametersInfo(
        SPI_SETFILTERKEYS,
        0,
        lp_FilterKeys_Param,
        1);

}

/****************************************************************************

    FUNCTION:    SaveStickeyKeys()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void SaveStickeyKeys()
{
    STICKYKEYS        *lp_StickyKeys_Param;
    lp_StickyKeys_Param = &StickyKeysParam;

    SystemParametersInfo(
        SPI_GETSTICKYKEYS,
        0,
        lp_StickyKeys_Param,
        0);

    SystemParametersInfo(
        SPI_SETSTICKYKEYS,
        0,
        lp_StickyKeys_Param,
        1);
}

/****************************************************************************

    FUNCTION:    SaveMouseKeys()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void SaveMouseKeys()
{
    MOUSEKEYS    *lp_MouseKeys_Param;
    lp_MouseKeys_Param = &MouseKeysParam;

    SystemParametersInfo(
        SPI_GETMOUSEKEYS,
        0,
        lp_MouseKeys_Param,
        0);

    SystemParametersInfo(
        SPI_SETMOUSEKEYS,
        0,
        lp_MouseKeys_Param,
        1);
}

/****************************************************************************

    FUNCTION:    SaveToggleKeys()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void SaveToggleKeys()
{
    TOGGLEKEYS    *lp_ToggleKeys_Param;
    lp_ToggleKeys_Param = &ToggleKeysParam;


    SystemParametersInfo(
        SPI_GETTOGGLEKEYS,
        0,
        lp_ToggleKeys_Param,
        0);

    SystemParametersInfo(
        SPI_SETTOGGLEKEYS,
        0,
        lp_ToggleKeys_Param,
        1);
}

/****************************************************************************

    FUNCTION:    SaveTimeOut()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void SaveTimeOut()
{
    ACCESSTIMEOUT        *lp_TimeOut_Param;

    lp_TimeOut_Param = &TimeOutParam;


    SystemParametersInfo(
        SPI_GETACCESSTIMEOUT,
        0,
        lp_TimeOut_Param,
        0);

    SystemParametersInfo(
        SPI_SETACCESSTIMEOUT,
        0,
        lp_TimeOut_Param,
        1);
}

/****************************************************************************

    FUNCTION:    SaveSerialKeys()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void SaveSerialKeys()
{
#ifdef MYSK
    MYSERIALKEYS           *lp_SerialKeys_Param;
    lp_SerialKeys_Param = &MySerialKeysParam;

    SystemParametersInfo(
        SPI_GETSERIALKEYS,
        sizeof(MySerialKeysParam),
        lp_SerialKeys_Param,
        0);

    SystemParametersInfo(
        SPI_SETSERIALKEYS,
        sizeof(MySerialKeysParam),
        lp_SerialKeys_Param,
        1);

#else

    SERIALKEYS           *lp_SerialKeys_Param;
	int Ret = 0;

    lp_SerialKeys_Param = &SerialKeysParam;
	

    Ret = SKEY_SystemParametersInfo(
        SPI_GETSERIALKEYS,
        sizeof(SerialKeysParam),
        lp_SerialKeys_Param, 0);

    Ret = SKEY_SystemParametersInfo(
        SPI_SETSERIALKEYS,
        sizeof(SerialKeysParam),
        lp_SerialKeys_Param, 1);
#endif

}
/****************************************************************************

    FUNCTION:    SaveShowSounds()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void SaveShowSounds()
{
    // ShowSounds unlike the others is a simple boolean value, so
    // SystemParametersInfo takes bool in wParam on SET, and
    // takes ptr to bool in lParam on GET.

    SystemParametersInfo(
        SPI_GETSHOWSOUNDS,
        0,
        &fShowSoundsOn,
        0);

    SystemParametersInfo(
        SPI_SETSHOWSOUNDS,
        fShowSoundsOn,
        0,
        1);
}

/****************************************************************************

    FUNCTION:    SaveSoundSentry()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void SaveSoundSentry()
{
    SOUNDSENTRY        *lp_SoundSentry_Param;
    lp_SoundSentry_Param = &SoundSentryParam;


    SystemParametersInfo(
        SPI_GETSOUNDSENTRY,
        0,
        lp_SoundSentry_Param,
        0);

    SystemParametersInfo(
        SPI_SETSOUNDSENTRY,
        0,
        lp_SoundSentry_Param,
        1);
}

/****************************************************************************

    FUNCTION:    SaveFeatures()

    PURPOSE:

    COMMENTS:


****************************************************************************/

void SaveFeatures()
{
    SaveFilterKeys();
    SaveStickeyKeys();
    SaveMouseKeys();
    SaveToggleKeys();
    SaveTimeOut();
    SaveSerialKeys();
    SaveSoundSentry();
    SaveShowSounds();
}

/////////////////////////////////////////////////////////////////////
#ifdef never
BOOL MySystemParametersInfo(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni)
    {
    BOOL b;
    switch( uiAction )
        {
        case SPI_GETSTICKYKEYS:
        case SPI_SETSTICKYKEYS:
                ((LPSTICKYKEYS)pvParam)->fStickyKeysOn =
                ((LPSTICKYKEYS)pvParam)->fStickyKeysOn ? -1 : 0;
                b = SystemParametersInfo( uiAction, uiParam, pvParam, fWinIni );
                ((LPSTICKYKEYS)pvParam)->fStickyKeysOn =
                ((LPSTICKYKEYS)pvParam)->fStickyKeysOn ? 1 : 0;
                return b;

        case SPI_GETMOUSEKEYS:
        case SPI_SETMOUSEKEYS:
                ((LPMOUSEKEYS)pvParam)->fMouseKeysOn =
                ((LPMOUSEKEYS)pvParam)->fMouseKeysOn ? -1 : 0;
                b = SystemParametersInfo( uiAction, uiParam, pvParam, fWinIni );
                ((LPMOUSEKEYS)pvParam)->fMouseKeysOn =
                ((LPMOUSEKEYS)pvParam)->fMouseKeysOn ? 1 : 0;
                return b;

        case SPI_GETTOGGLEKEYS:
        case SPI_SETTOGGLEKEYS:
                ((LPTOGGLEKEYS)pvParam)->fToggleKeysOn =
                ((LPTOGGLEKEYS)pvParam)->fToggleKeysOn ? -1 : 0;
                b = SystemParametersInfo( uiAction, uiParam, pvParam, fWinIni );
                ((LPTOGGLEKEYS)pvParam)->fToggleKeysOn =
                ((LPTOGGLEKEYS)pvParam)->fToggleKeysOn ? 1 : 0;
                return b;

        case SPI_GETFILTERKEYS:
        case SPI_SETFILTERKEYS:
                ((LPFILTERKEYS)pvParam)->fFilterKeysOn =
                ((LPFILTERKEYS)pvParam)->fFilterKeysOn ? -1 : 0;
                b = SystemParametersInfo( uiAction, uiParam, pvParam, fWinIni );
                ((LPFILTERKEYS)pvParam)->fFilterKeysOn =
                ((LPFILTERKEYS)pvParam)->fFilterKeysOn ? 1 : 0;
                return b;

        case SPI_GETSOUNDSENTRY:
        case SPI_SETSOUNDSENTRY:
                ((LPSOUNDSENTRY)pvParam)->fSoundSentryOn =
                ((LPSOUNDSENTRY)pvParam)->fSoundSentryOn ? -1 : 0;
                b = SystemParametersInfo( uiAction, uiParam, pvParam, fWinIni );
                ((LPSOUNDSENTRY)pvParam)->fSoundSentryOn =
                ((LPSOUNDSENTRY)pvParam)->fSoundSentryOn ? 1 : 0;
                return b;

        case SPI_GETACCESSTIMEOUT:
        case SPI_SETACCESSTIMEOUT:
                ((LPACCESSTIMEOUT)pvParam)->fTimeOutOn =
                ((LPACCESSTIMEOUT)pvParam)->fTimeOutOn ? -1 : 0;
                b = SystemParametersInfo( uiAction, uiParam, pvParam, fWinIni );
                ((LPACCESSTIMEOUT)pvParam)->fTimeOutOn =
                ((LPACCESSTIMEOUT)pvParam)->fTimeOutOn ? 1 : 0;
                return b;

        case SPI_GETSERIALKEYS:
        case SPI_SETSERIALKEYS:
                ((LPSERIALKEYS)pvParam)->fSerialKeysOn =
                ((LPSERIALKEYS)pvParam)->fSerialKeysOn ? -1 : 0;
                b = SystemParametersInfo( uiAction, uiParam, pvParam, fWinIni );
                ((LPSERIALKEYS)pvParam)->fSerialKeysOn =
                ((LPSERIALKEYS)pvParam)->fSerialKeysOn ? 1 : 0;
                return b;

#ifdef NEVER
        case SPI_GETACCESSHIGHCONTRAST:
        case SPI_SETACCESSHIGHCONTRAST:
                ((LPACCESSHIGHCONTRAST)pvParam)->fHighContrastOn =
                ((LPACCESSHIGHCONTRAST)pvParam)->fHighContrastOn ? -1 : 0;
                b = SystemParametersInfo( uiAction, uiParam, pvParam, fWinIni );
                ((LPACCESSHIGHCONTRAST)pvParam)->fHighContrastOn =
                ((LPACCESSHIGHCONTRAST)pvParam)->fHighContrastOn ? 1 : 0;
                return b;
#endif
        default:
                return SystemParametersInfo( uiAction, uiParam, pvParam, fWinIni );
        }
    }
#endif
