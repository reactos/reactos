/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 services/dd/mpu401/dll/mpu401.c
 * PURPOSE:              MPU-401 MIDI driver WINMM interface
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Sept 28, 2003: Created
 */

#include <windows.h>
typedef UINT *LPUINT;
#include <mmsystem.h>
#include "messages.h"


#define DBG printf


// Erm... This is in mmsystem i think:
LONG APIENTRY DefDriverProc(DWORD  dwDriverIdentifier,
                            HANDLE hDriver,
                            UINT   message,  // Bug in ptypes32.h
                            LONG   lParam1,
                            LONG   lParam2);




STATIC int MessageLength(BYTE b)
{
    if (b > 0xf8)   return 1;

    switch(b)
    {
        case 0xf0 : case 0xf4 : case 0xf5 : case 0xf6 : case 0xf7 :
            return 1;
        case 0xf1 : case 0xf3 :
            return 2;
        case 0xf2 :
            return 3;
    }

    switch(b & 0xf0)
    {
        case 0x80 : case 0x90 : case 0xa0 : case 0xb0 : case 0xe0 :
            return 2;
    }

    return 0;   // must be a status byte
}


DWORD APIENTRY modMessage(DWORD id, DWORD msg, DWORD dwUser,
                          DWORD dwParam1, DWORD dwParam2)
{
    switch(msg)
    {
        case MODM_GETNUMDEVS :
            DBG("MODM_GETNUMDEVS\n");
            break;

        case MODM_GETDEVCAPS :
            DBG("MODM_GETDEVCAPS\n");
            break;

        case MODM_OPEN :
            DBG("MODM_OPEN\n");
            break;

        case MODM_CLOSE :
            DBG("MODM_CLOSE\n");
            break;

        case MODM_DATA :
        /*
            MODM_DATA requests that the driver process a short MIDI message.
            
            PARAMETERS
                id :        ?
                dwUser :    ?
                dwParam1 :  MIDI message packed into a 32-bit number
                dwParam2 :  ?

            RETURN VALUES
                ???
        */
        {
            int i;
            BYTE b[4];
            DBG("MODM_DATA\n");
            for (i = 0; i < 4; i ++)
            {
                b[i] = (BYTE)(dwParam1 % 256);
                dwParam1 /= 256;
            }
            // midiOutWrite(data, length, client?)
// somehow we need to keep track of running status
//            return midiOutWrite(b, modMIDIlength((PMIDIALLOC)dwUser, b[0]),
//                                (PMIDIALLOC)dwUser);
        }

        case MODM_LONGDATA :
            DBG("MODM_LONGDATA\n");
            break;

        case MODM_RESET :
            DBG("MODM_RESET\n");
            break;

        case MODM_SETVOLUME :
            DBG("MODM_SETVOLUME\n");
            break;

        case MODM_GETVOLUME :
            DBG("MODM_GETVOLUME\n");
            break;

        case MODM_CACHEPATCHES :
            DBG("MODM_CACHEPATCHES\n");
            break;

        case MODM_CACHEDRUMPATCHES :
            DBG("MODM_CACHEDRUMPATCHES\n");
            break;
    }

//    return MMSYSERR_NOT_SUPPORTED;
}



LRESULT DriverProc(DWORD dwDriverID, HDRVR hDriver, UINT uiMessage,
                   LPARAM lParam1, LPARAM lParam2)
/*
    ROUTINE
        DriverProc

    PURPOSE
        Process driver messages

    PARAMETERS
        dwDriverID :    Identifier of installable driver
        hDriver :       Handle of the installable driver instance
        uiMessage :     Which operation to perform
        lParam1 :       Message-dependent
        lParam2 :       Message-dependent

    NOTES
        For parameters that aren't listed in the messages, they are not
        used.
        
        Loading and unloading messages occur in this order:
        DRV_LOAD - DRV_ENABLE - DRV_OPEN
        DRV_CLOSE - DRV_DISABLE - DRV_FREE
*/
{
    switch(uiMessage)
    {
        case DRV_LOAD :
        /*
            DRV_LOAD notifies the driver that it has been loaded. It should
            then make sure it can function properly.

            PARAMETERS
                (No Parameters)

            RETURN VALUES
                Non-zero if successful
                Zero if not
        */
            DBG("DRV_LOAD\n");
            break;

        case DRV_FREE :
        /*
            DRV_FREE notifies the driver that it is being unloaded. It
            should make sure it releases any memory or other resources.

            PARAMETERS
                hDriver :       Hande of the installable driver instance

            RETURN VALUES
                Nothing
        */
            DBG("DRV_FREE\n");
            break;

        case DRV_OPEN :
        /*
            DRV_OPEN directs the driver to open a new instance.

            PARAMETERS
                dwDriverID :    Identifier of the installable driver
                hDriver :       Handle of the installable driver instance
                lParam1 :       Wide string specifying configuration
                                information, or this can be NULL
                lParam2 :       32-bit driver-specific data
            
            RETURN VALUES
                Non-zero if successful
                Zero if not
        */
            DBG("DRV_OPEN\n");
            break;

        case DRV_CLOSE :
        /*
            DRV_CLOSE directs the driver to close the specified instance.

            PARAMETERS
                dwDriverID :    Identifier of the installable driver
                hDriver :       Handle of the installable driver instance
                lParam1 :       32-bit value passed from DriverClose()
                lParam2 :       32-bit value passed from DriverClose()

            RETURN VALUES
                Non-zero if successful
                Zero if not
        */
            DBG("DRV_CLOSE\n");
            break;

        case DRV_ENABLE :
        /*
            DRV_ENABLE enables the driver (as if you didn't see THAT one
            coming!)

            PARAMETERS
                hDriver :       Handle of the installable driver instance
            
            RETURN VALUES
                Nothing
        */
            DBG("DRV_ENABLE\n");
            break;

        case DRV_DISABLE :
        /*
            DRV_DISABLE disables the driver - see above comment ;) This
            message comes before DRV_FREE.

            PARAMETERS
                hDriver :       Handle of the installable driver instance

            RETURN VALUES
                Nothing
        */
            DBG("DRV_DISABLE\n");
            break;

        case DRV_QUERYCONFIGURE :
        /*
            DRV_QUERYCONFIGURE asks the driver if it supports custom
            configuration.
            
            PARAMETERS
                dwDriverID :    Identifier of the installable driver
                hDriver :       Handle of the installable driver instance

            RETURN VALUES
                Non-zero to indicate the driver can display a config dialog
                Zero if not
        */
            DBG("DRV_QUERYCONFIGURE\n");
            break;

        case DRV_CONFIGURE :
        /*
            DRV_CONFIGURE requests the driver to display a configuration
            dialog box.
            
            PARAMETERS
                dwDriverID :    Identified of the installable driver
                hDriver :       Handle of the installable driver instance
                lParam1 :       Handle of the parent window of the dialog
                lParam2 :       Address of a DRVCONFIGINFO, or NULL

            RETURN VALUES
                DRVCNF_OK       The configuration was successful
                DRVCNF_CANCEL   The user cancelled the dialog box
                DRVCNF_RESTART  The configuration requires a reboot
        */
            DBG("DRV_CONFIGURE\n");
            break;

        case DRV_INSTALL :
        /*
            DRV_INSTALL notifies the driver that it is being installed.

            PARAMETERS
                dwDriverID :    Identifier of the installable driver
                hDriver :       Handle of the installable driver instance
                lParam2 :       Address of a DEVCONFIGINFO, or NULL

            RETURN VALUES
                DRVCNF_OK       The configuration was successful
                DRVCNF_CANCEL   The user cancelled the dialog box
                DRVCNF_RESTART  The configuration requires a reboot
        */
            DBG("DRV_INSTALL\n");
            break;

        case DRV_REMOVE :
        /*
            DRV_REMOVE notifies the driver that it is being removed from the
            system.
            
            PARAMETERS
                dwDriverID :    Identifier of the installable driver
                hDriver :       Handle of the installable driver instance
            
            RETURN VALUES
                Nothing
        */
            DBG("DRV_REMOVE\n");
            break;

        case DRV_POWER :
        /*
            DRV_POWER notifies the driver that power is being turned on/off.
            
            PARAMETERS
                dwDriverID :    Identifier of the installable driver
                hDriver :       Handle of the installable driver instance

            RETURN VALUES
                Nothing
        */
            DBG("DRV_POWER\n");

        case DRV_EXITSESSION :
        /*
            DRV_EXITSESSION notifies the driver that Windows is shutting down.
            
            PARAMETERS
                dwDriverID :    Identifier of the installable driver
                hDriver :       Handle of the installable driver instance

            RETURN VALUES
                Nothing
        */

//        case DRV_PNPINSTALL : break;
        default :
            return DefDriverProc(dwDriverID, hDriver, uiMessage, lParam1, lParam2);
    }

    return DefDriverProc(dwDriverID, hDriver, uiMessage, lParam1, lParam2);
}


BOOL CALLBACK DProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
//    if (uMsg == WM_INITDIALOG)
//        return -1;

    return 0;
}


void Test()
{
//    HWND Dlg = CreateDialog(GetModuleHandle(NULL), "Config", NULL, DProc);
    HWND Z = CreateWindow("Static", "", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 20, 20, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(1001), Z, DProc) == -1)
        MessageBox(NULL, "Error", "Error", MB_OK | MB_TASKMODAL);
}
