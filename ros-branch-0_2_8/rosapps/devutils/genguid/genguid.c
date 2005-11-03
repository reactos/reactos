/*
 *  genguid utility for WINE and ReactOS
 *
 *  Copyright 2003 Jonathan Wilson
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

int main(int argc, char *argv[])
{
	GUID m_guid;
	m_guid = GUID_NULL;
	int arg;
	HRESULT result;
	char *strfmt = "";
	if (argc < 2) {
		printf("usage: %s n\n",argv[0]);
		printf("n = format of output\n");
		printf("values are:\n");
		printf("1 = IMPLEMENT_OLECREATE defintion\n");
		printf("2 = DEFINE_GUID definition\n");
		printf("3 = static const GUID definition\n");
		printf("4 = registry format\n");
		printf("5 = uuidgen.exe format\n");
		return 1;
	}
	arg = atoi(argv[1]);
	if ((arg > 5) || (arg <= 0)) {
		printf("invalid argument\n");
		return 1;
	}
	if (CoInitialize(NULL) != S_OK)
	{
		printf("Unable to initalize OLE libraries\n");
		return 1;
	}
	result = CoCreateGuid(&m_guid);
	if (result != S_OK) {
		printf("Unable to create GUID\n");
		CoUninitialize();
		return 1;
	}
	switch (arg) {
	case 1:
	strfmt = "// {%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}\r\nIMPLEMENT_OLECREATE(<<class>>, <<external_name>>, \r\n0x%lx, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x);\r\n";
	break;
	case 2:
	strfmt = "// {%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}\r\nDEFINE_GUID(<<name>>, \r\n0x%lx, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x);\r\n";
	break;
	case 3:
	strfmt = "// {%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}\r\nstatic const GUID <<name>> = \r\n{ 0x%lx, 0x%x, 0x%x, { 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x } };\r\n";
	break;
	case 4:
	strfmt = "{%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}\r\n";
	break;
	case 5:
	strfmt = "%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X\r\n";
	break;
	}
	printf(strfmt,m_guid.Data1,m_guid.Data2,m_guid.Data3,m_guid.Data4[0],
	m_guid.Data4[1],m_guid.Data4[2],m_guid.Data4[3],m_guid.Data4[4],m_guid.Data4[5],
	m_guid.Data4[6],m_guid.Data4[7],m_guid.Data1,m_guid.Data2,m_guid.Data3,m_guid.Data4[0],
	m_guid.Data4[1],m_guid.Data4[2],m_guid.Data4[3],m_guid.Data4[4],m_guid.Data4[5],
	m_guid.Data4[6],m_guid.Data4[7]);
	CoUninitialize();
	return 0;
}


