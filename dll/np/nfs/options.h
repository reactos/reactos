/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#ifndef __NFS41_NP_OPTIONS_H__
#define __NFS41_NP_OPTIONS_H__


#define MOUNT_OPTION_BUFFER_SECRET ('n4')

/* MOUNT_OPTION_BUFFER
 *   The mount options buffer received by NPAddConnection3
 * via NETRESOURCE.lpComment. To avoid interpreting a normal
 * comment string as mount options, a NULL and secret number
 * are expected at the front. */
typedef struct _MOUNT_OPTION_BUFFER {
	USHORT	Zero;	/* = 0 */
	USHORT	Secret;	/* = 'n4' */
	ULONG	Length;
	BYTE	Buffer[1];
} MOUNT_OPTION_BUFFER, *PMOUNT_OPTION_BUFFER;

/* CONNECTION_BUFFER
 *   The connection information as sent to the driver via
 * IOCTL_NFS41_ADDCONN. The buffer contains the connection name
 * followed by any extended attributes for mount options. */
typedef struct _CONNECTION_BUFFER {
	USHORT	NameLength;	/* length of connection filename */
	USHORT	EaPadding;	/* 0-3 bytes of padding to put EaBuffer
						 * on a ULONG boundary */
	ULONG	EaLength;	/* length of EaBuffer */
	BYTE	Buffer[1];
} CONNECTION_BUFFER, *PCONNECTION_BUFFER;

/* CONNECTION_INFO
 *   Used in NPAddConnection3 to encapsulate the formation of
 * the connection buffer. */
typedef struct _CONNECTION_INFO {
	PMOUNT_OPTION_BUFFER	Options;
	ULONG					BufferSize;
	PCONNECTION_BUFFER		Buffer;
} CONNECTION_INFO, *PCONNECTION_INFO;

#define MAX_CONNECTION_BUFFER_SIZE(EaSize) ( \
	sizeof(CONNECTION_BUFFER) + MAX_PATH + (EaSize) )


/* options.c */
DWORD InitializeConnectionInfo(
	IN OUT PCONNECTION_INFO Connection,
	IN PMOUNT_OPTION_BUFFER Options,
	OUT LPWSTR *ConnectionName);

void FreeConnectionInfo(
	IN OUT PCONNECTION_INFO Connection);

/* MarshallConnectionInfo
 *   Prepares the CONNECTION_BUFFER for transmission to the driver
 * by copying the extended attributes into place and updating the
 * lengths accordingly. */
void MarshalConnectionInfo(
	IN OUT PCONNECTION_INFO Connection);


#endif /* !__NFS41_NP_OPTIONS_H__ */
