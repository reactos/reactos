/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/
#include "diskio.h"
#include "../FAT.h"
#include <stdio.h>

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and image file handles.  */

HANDLE driveHandle[1] = {INVALID_HANDLE_VALUE};
const int driveHandleCount = sizeof(driveHandle) / sizeof(FILE*);

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{
	if(pdrv == 0) // only one drive (image file) supported atm.
	{
		if(driveHandle[0]!=INVALID_HANDLE_VALUE)
			return 0;

		driveHandle[0]=CreateFile(imageFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL /* | FILE_FLAG_RANDOM_ACCESS */, NULL);

		if(driveHandle[0]!=INVALID_HANDLE_VALUE)
			return 0;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{
	if(pdrv < driveHandleCount)
	{
		if(driveHandle[pdrv] != INVALID_HANDLE_VALUE)
			return 0;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
)
{
	DWORD result;

	if(pdrv < driveHandleCount)
	{
		if(driveHandle[pdrv] != INVALID_HANDLE_VALUE)
		{
			if(SetFilePointer(driveHandle[pdrv], sector * 512, NULL, SEEK_SET) == INVALID_SET_FILE_POINTER)
				return RES_ERROR;

			if(!ReadFile(driveHandle[pdrv], buff, 512 * count, &result, NULL))
				return RES_ERROR;

			if(result == (512 * count))
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
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
	DWORD result;

	if(pdrv < driveHandleCount)
	{
		if(driveHandle[pdrv] != INVALID_HANDLE_VALUE)
		{
			if(SetFilePointer(driveHandle[pdrv], sector * 512, NULL, SEEK_SET) == INVALID_SET_FILE_POINTER)
				return RES_ERROR;

			if(!WriteFile(driveHandle[pdrv], buff, 512 * count, &result, NULL))
				return RES_ERROR;

			if(result == (512 * count))
				return RES_OK;

			return RES_ERROR;
		}
	}

	return RES_PARERR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	if(pdrv < driveHandleCount)
	{
		if(driveHandle[pdrv] != INVALID_HANDLE_VALUE)
		{
			switch(cmd)
			{
			case CTRL_SYNC:
				return RES_OK;
			case GET_SECTOR_SIZE:
				*(DWORD*)buff = 512;
				return RES_OK;
			case GET_BLOCK_SIZE:
				*(DWORD*)buff = 512;
				return RES_OK;
			case GET_SECTOR_COUNT:
				{
					*(DWORD*)buff = GetFileSize(driveHandle[pdrv], NULL) / 512;
				}
				return RES_OK;
			case SET_SECTOR_COUNT:
				{
					SetFilePointer(driveHandle[pdrv], (*(DWORD*)buff)*512, NULL, SEEK_SET);
					SetEndOfFile(driveHandle[pdrv]);
				}
			}
		}
	}

	return RES_PARERR;
}
#endif
