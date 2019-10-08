/*
 * PROJECT:     ReactOS Xbox miniport video driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     I2C SMBus routines
 * COPYRIGHT:   Copyright 2004 Gé van Geldorp
 *              Copyright 2004 Filip Navara
 *              Copyright 2019 Stanislav Motylkov (x86corez@gmail.com)
 */

/* INCLUDES *******************************************************************/

#include "xboxvmp.h"

#include <debug.h>
#include <dpfilter.h>

/* PUBLIC AND PRIVATE FUNCTIONS ***********************************************/

static
BOOLEAN
ReadfromSMBus(
    UCHAR Address,
    UCHAR bRegister,
    UCHAR Size,
    ULONG *Data_to_smbus)
{
    int nRetriesToLive = 50;

    while ((VideoPortReadPortUshort((PUSHORT) (I2C_IO_BASE + 0)) & 0x0800) != 0)
    {
        ; /* Franz's spin while bus busy with any master traffic */
    }

    while (nRetriesToLive-- != 0)
    {
        UCHAR b;
        int temp;

        VideoPortWritePortUchar((PUCHAR) (I2C_IO_BASE + 4), (Address << 1) | 1);
        VideoPortWritePortUchar((PUCHAR) (I2C_IO_BASE + 8), bRegister);

        temp = VideoPortReadPortUshort((PUSHORT) (I2C_IO_BASE + 0));
        VideoPortWritePortUshort((PUSHORT) (I2C_IO_BASE + 0), temp); /* clear down all preexisting errors */

        switch (Size)
        {
            case 4:
            {
                VideoPortWritePortUchar((PUCHAR) (I2C_IO_BASE + 2), 0x0d); /* DWORD modus ? */
                break;
            }

            case 2:
            {
                VideoPortWritePortUchar((PUCHAR) (I2C_IO_BASE + 2), 0x0b); /* WORD modus */
                break;
            }

            default:
            {
                VideoPortWritePortUchar((PUCHAR) (I2C_IO_BASE + 2), 0x0a); /* BYTE */
            }
        }

        b = 0;

        while ((b & 0x36) == 0)
        {
            b = VideoPortReadPortUchar((PUCHAR) (I2C_IO_BASE + 0));
        }

        if ((b & 0x24) != 0)
        {
            ERR_(IHVVIDEO, "I2CTransmitByteGetReturn error %x\n", b);
        }

        if ((b & 0x10) == 0)
        {
            ERR_(IHVVIDEO, "I2CTransmitByteGetReturn no complete, retry\n");
        }
        else
        {
            switch (Size)
            {
                case 4:
                {
                    VideoPortReadPortUchar((PUCHAR) (I2C_IO_BASE + 6));
                    VideoPortReadPortUchar((PUCHAR) (I2C_IO_BASE + 9));
                    VideoPortReadPortUchar((PUCHAR) (I2C_IO_BASE + 9));
                    VideoPortReadPortUchar((PUCHAR) (I2C_IO_BASE + 9));
                    VideoPortReadPortUchar((PUCHAR) (I2C_IO_BASE + 9));
                    break;
                }

                case 2:
                {
                    *Data_to_smbus = VideoPortReadPortUshort((PUSHORT) (I2C_IO_BASE + 6));
                    break;
                }

                default:
                {
                    *Data_to_smbus = VideoPortReadPortUchar((PUCHAR) (I2C_IO_BASE + 6));
                }
            }

            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
I2CTransmitByteGetReturn(
    UCHAR bPicAddressI2cFormat,
    UCHAR bDataToWrite,
    ULONG *Return)
{
    return ReadfromSMBus(bPicAddressI2cFormat, bDataToWrite, 1, Return);
}

/* EOF */
