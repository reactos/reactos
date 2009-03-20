/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 dll/win32/mmdrv/mme.c
 * PURPOSE:              Multimedia User Mode Driver (MME Interface)
 * PROGRAMMER:           Andrew Greenwood
 *                       Aleksey Bragin
 * UPDATE HISTORY:
 *                       Jan 14, 2007: Rewritten and tidied up
 */

#include <mmdrv.h>

/*
    Sends a message to the client (application), such as WOM_DONE. This
    is just a wrapper around DriverCallback which translates the
    parameters appropriately.
*/

BOOL
NotifyClient(
    SessionInfo* session_info,
    DWORD message,
    DWORD_PTR parameter1,
    DWORD_PTR parameter2)
{
    return DriverCallback(session_info->callback,
                          HIWORD(session_info->flags),
                          session_info->mme_handle,
                          message,
                          session_info->app_user_data,
                          parameter1,
                          parameter2);
}



/*
    MME Driver Entrypoint
    Wave Output
*/

APIENTRY DWORD
wodMessage(
    UINT device_id,
    UINT message,
    DWORD_PTR private_handle,
    DWORD_PTR parameter1,
    DWORD_PTR parameter2)
{
    switch ( message )
    {
        /* http://www.osronline.com/ddkx/w98ddk/mmedia_4p80.htm */
        case WODM_GETNUMDEVS :
            DPRINT("WODM_GETNUMDEVS\n");
            return GetDeviceCount(WaveOutDevice);

        /* http://www.osronline.com/ddkx/w98ddk/mmedia_4p6h.htm */
        case WODM_GETDEVCAPS :
            DPRINT("WODM_GETDEVCAPS\n");
            return GetDeviceCapabilities(WaveOutDevice,
                                         device_id,
                                         parameter1,
                                         parameter2);

        /* http://www.osronline.com/ddkx/w98ddk/mmedia_4p85.htm */
        case WODM_OPEN :
        {
            WAVEOPENDESC* open_desc = (WAVEOPENDESC*) parameter1;
            DPRINT("WODM_OPEN\n");

            if ( parameter2 && WAVE_FORMAT_QUERY )
                return QueryWaveFormat(WaveOutDevice, open_desc->lpFormat);
            else
                return OpenDevice(WaveOutDevice,
                                  device_id,
                                  open_desc,
                                  parameter2,
                                  private_handle);
        }

        /* http://www.osronline.com/ddkx/w98ddk/mmedia_4p6g.htm */
        case WODM_CLOSE :
        {
            DPRINT("WODM_CLOSE\n");
            return CloseDevice(private_handle);
        }

        /* http://www.osronline.com/ddkx/w98ddk/mmedia_4p9w.htm */
        case WODM_WRITE :
        {
            DPRINT("WODM_WRITE\n");
            return WriteWaveBuffer(private_handle,
                                   (PWAVEHDR) parameter1,
                                   parameter2);
        }

        /* http://www.osronline.com/ddkx/w98ddk/mmedia_4p86.htm */
        case WODM_PAUSE :
        {
            DPRINT("WODM_PAUSE\n");
            return HandleBySessionThread(private_handle, message, 0);
        }

        /* http://www.osronline.com/ddkx/w98ddk/mmedia_4p89.htm */
        case WODM_RESTART :
        {
            DPRINT("WODM_RESTART\n");
            return HandleBySessionThread(private_handle, message, 0);
        }

        /* http://www.osronline.com/ddkx/w98ddk/mmedia_4p88.htm */
        case WODM_RESET :
        {
            DPRINT("WODM_RESET\n");
            return HandleBySessionThread(private_handle, message, 0);
        }

        /* http://www.osronline.com/ddkx/w98ddk/mmedia_4p83.htm */
#if 0
        case WODM_GETPOS :
        {
            DPRINT("WODM_GETPOS\n");
            return GetPosition(private_handle,
                               (PMMTIME) parameter1,
                               parameter2);
        }
#endif

        /* http://www.osronline.com/ddkx/w98ddk/mmedia_4p6f.htm */
        case WODM_BREAKLOOP :
        {
            DPRINT("WODM_BREAKLOOP\n");
            return HandleBySessionThread(private_handle, message, 0);
        }

        /* TODO: Others */
    }

    DPRINT("Unsupported message\n");
    return MMSYSERR_NOTSUPPORTED;
}
