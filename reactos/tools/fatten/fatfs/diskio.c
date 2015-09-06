/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/
#include "diskio.h"
#include <stdio.h>

extern char* imageFileName;

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and image file handles.  */

FILE* driveHandle[1] = { NULL };
const int driveHandleCount = sizeof(driveHandle) / sizeof(FILE*);

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
    BYTE pdrv        /* Physical drive nmuber (0..) */
    )
{
    if (pdrv == 0) /* only one drive (image file) supported atm. */
    {
        if (driveHandle[0] != NULL)
            return 0;

        driveHandle[0] = fopen(imageFileName, "r+b");

        if (driveHandle[0] != NULL)
            return 0;
    }
    return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(
    BYTE pdrv		/* Physical drive nmuber (0..) */
    )
{
    if (pdrv < driveHandleCount)
    {
        if (driveHandle[pdrv] != NULL)
            return 0;
    }
    return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(
    BYTE pdrv,		/* Physical drive nmuber (0..) */
    BYTE *buff,		/* Data buffer to store read data */
    DWORD sector,	/* Sector address (LBA) */
    UINT count		/* Number of sectors to read (1..128) */
    )
{
    DWORD result;

    if (pdrv < driveHandleCount)
    {
        if (driveHandle[pdrv] != NULL)
        {
            if (fseek(driveHandle[pdrv], sector * 512, SEEK_SET))
                return RES_ERROR;

            result = fread(buff, 512, count, driveHandle[pdrv]);

            if (result == count)
                return RES_OK;

            return RES_ERROR;
        }
    }

    return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write(
    BYTE pdrv,			/* Physical drive nmuber (0..) */
    const BYTE *buff,	/* Data to be written */
    DWORD sector,		/* Sector address (LBA) */
    UINT count			/* Number of sectors to write (1..128) */
    )
{
    DWORD result;

    if (pdrv < driveHandleCount)
    {
        if (driveHandle[pdrv] != NULL)
        {
            if (fseek(driveHandle[pdrv], sector * 512, SEEK_SET))
                return RES_ERROR;

            result = fwrite(buff, 512, count, driveHandle[pdrv]);
            return RES_ERROR;

            if (result != (512 * count))
                return RES_ERROR;

            return RES_OK;
        }
    }

    return RES_PARERR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl(
    BYTE pdrv,		/* Physical drive nmuber (0..) */
    BYTE cmd,		/* Control code */
    void *buff		/* Buffer to send/receive control data */
    )
{
    if (pdrv < driveHandleCount)
    {
        if (driveHandle[pdrv] != NULL)
        {
            switch (cmd)
            {
            case CTRL_SYNC:
                fflush(driveHandle[pdrv]);
                return RES_OK;
            case GET_SECTOR_SIZE:
                *(DWORD*)buff = 512;
                return RES_OK;
            case GET_BLOCK_SIZE:
                *(DWORD*)buff = 512;
                return RES_OK;
            case GET_SECTOR_COUNT:
                fseek(driveHandle[pdrv], 0, SEEK_END);
                *(DWORD*)buff = ftell(driveHandle[pdrv]) / 512;
                return RES_OK;
            case SET_SECTOR_COUNT:
            {
                int count = *(DWORD*)buff;
                long size;

                fseek(driveHandle[pdrv], 0, SEEK_END);
                size = ftell(driveHandle[pdrv]) / 512;

                if (size < count)
                {
                    if (fseek(driveHandle[pdrv], count * 512 - 1, SEEK_SET))
                        return RES_ERROR;

                    fwrite(buff, 1, 1, driveHandle[pdrv]);

                    return RES_OK;
                }
                else
                {
                    // SHRINKING NOT IMPLEMENTED
                    return RES_OK;
                }
            }
            }
        }
    }

    return RES_PARERR;
}
#endif
