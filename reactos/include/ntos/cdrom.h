/* $Id: cdrom.h,v 1.1 2002/04/10 17:00:52 ekohl Exp $
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


#define IOCTL_CDROM_GET_DRIVE_GEOMETRY   CTL_CODE(FILE_DEVICE_CD_ROM, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)


#endif /* __INCLUDE_NTOS_CDROM_H */

/* EOF */
