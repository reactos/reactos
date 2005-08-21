/*
 *  Based on the genguid utility for WINE and ReactOS
 *
 *  Copyright 2003 Jonathan Wilson
 *  Copyright 2005 Steven Edwards
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <objbase.h>
#include <stdio.h>

static HRESULT (*pCoInitialize)(PVOID);
static void (*pCoUninitialize)(void);
static HRESULT (*pCoCreateGuid)(GUID*);

void gen_guid()
{
	GUID m_guid;
	HRESULT result;
	char *strfmt = "";
	HMODULE olelib;

    /* Load ole32. We will need it later on */
	olelib = LoadLibraryA( "ole32.dll" );

	if (olelib != NULL)
		pCoInitialize = (HRESULT (*)(void*))GetProcAddress( olelib, "CoInitialize" );
		pCoUninitialize = (void (*)())GetProcAddress( olelib, "CoUninitialize" );
		pCoCreateGuid = (HRESULT (*)(GUID*))GetProcAddress( olelib, "CoCreateGuid" );

	if (pCoInitialize(NULL) != S_OK)
	{
		printf("Unable to initalize OLE libraries\n");
	}
	result = pCoCreateGuid(&m_guid);
	if (result != S_OK) {
		printf("Unable to create GUID\n");
		pCoUninitialize();
	}
	strfmt = "%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X\r\n";
	printf(strfmt,m_guid.Data1,m_guid.Data2,m_guid.Data3,m_guid.Data4[0],
	m_guid.Data4[1],m_guid.Data4[2],m_guid.Data4[3],m_guid.Data4[4],m_guid.Data4[5],
	m_guid.Data4[6],m_guid.Data4[7],m_guid.Data1,m_guid.Data2,m_guid.Data3,m_guid.Data4[0],
	m_guid.Data4[1],m_guid.Data4[2],m_guid.Data4[3],m_guid.Data4[4],m_guid.Data4[5],
	m_guid.Data4[6],m_guid.Data4[7]);
	pCoUninitialize();
}
