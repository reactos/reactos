/* $Id: cdrom.h,v 1.3 2002/09/17 20:35:22 hbirr Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntos/cdrom.h
 * PURPOSE:      CD-ROM related definitions used by all the parts of the system
 * PROGRAMMER:   Eric Kohl <ekohl@rz-online.de>
 * UPDATE HISTORY: 
 *               10/04/2002: Created
 */

#ifndef __INCLUDE_NTOS_CDROM_H
#define __INCLUDE_NTOS_CDROM_H

#define IOCTL_CDROM_READ_TOC		CTL_CODE(FILE_DEVICE_CD_ROM, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_LAST_SESSION    CTL_CODE(FILE_DEVICE_CD_ROM, 0x000E, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_DRIVE_GEOMETRY	CTL_CODE(FILE_DEVICE_CD_ROM, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)

#define MAXIMUM_NUMBER_TRACKS		100
#define MAXIMUM_CDROM_SIZE              804

typedef struct _TRACK_DATA {
  UCHAR  Reserved;
  UCHAR  Control : 4;
  UCHAR  Adr : 4;
  UCHAR  TrackNumber;
  UCHAR  Reserved1;
  UCHAR  Address[4];
} TRACK_DATA, *PTRACK_DATA;

typedef struct _CDROM_TOC {
  UCHAR  Length[2];
  UCHAR  FirstTrack;
  UCHAR  LastTrack;
  TRACK_DATA  TrackData[MAXIMUM_NUMBER_TRACKS];
} CDROM_TOC, *PCDROM_TOC;

#define CDROM_TOC_SIZE sizeof(CDROM_TOC)



#endif /* __INCLUDE_NTOS_CDROM_H */

/* EOF */
