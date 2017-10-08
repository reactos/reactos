/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserEnumDisplaySettings
 * PROGRAMMERS:
 */

#include <win32nt.h>



static struct
{
	DEVMODEW devmode;
	CHAR buffer[0xffff];
} data;

START_TEST(NtUserEnumDisplaySettings)
{
	UNICODE_STRING usDeviceName;
	WCHAR szName[] = L"DISPLAY";
	NTSTATUS Status;
	INT i;

	SetLastError(ERROR_SUCCESS);
	Status = NtUserEnumDisplaySettings(NULL, 0, 0, 0);
	TEST(Status == STATUS_ACCESS_VIOLATION);
	TEST(GetLastError() == ERROR_SUCCESS);

	data.devmode.dmDriverExtra = 0;
	for (i = 0; i < 2 * sizeof(DEVMODEW); i++)
	{
		data.devmode.dmSize = i;
		Status = NtUserEnumDisplaySettings(NULL, 1000, (DEVMODEW*)&data, 0);
		if (i != sizeof(DEVMODEW))
		{
			TEST(Status == STATUS_BUFFER_TOO_SMALL);
		}
	}
	TEST(GetLastError() == ERROR_SUCCESS);

	usDeviceName.Buffer = NULL;
	usDeviceName.Length = 0;
	usDeviceName.MaximumLength = 0;
	Status = NtUserEnumDisplaySettings(&usDeviceName, ENUM_CURRENT_SETTINGS, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_BUFFER_TOO_SMALL);
	Status = NtUserEnumDisplaySettings(&usDeviceName, -4, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_BUFFER_TOO_SMALL);

	data.devmode.dmSize = sizeof(DEVMODEW);
	data.devmode.dmDriverExtra = 0xffff;
	Status = NtUserEnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_SUCCESS);

	data.devmode.dmSize = sizeof(DEVMODEW);
	data.devmode.dmDriverExtra = 0;
	Status = NtUserEnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_SUCCESS);

	usDeviceName.Buffer = NULL;
	usDeviceName.Length = 0;
	usDeviceName.MaximumLength = 0;
	Status = NtUserEnumDisplaySettings(&usDeviceName, ENUM_CURRENT_SETTINGS, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_INVALID_PARAMETER_1);
	Status = NtUserEnumDisplaySettings(&usDeviceName, -4, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_INVALID_PARAMETER_1);

	Status = NtUserEnumDisplaySettings(NULL, 0, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_SUCCESS);
	Status = NtUserEnumDisplaySettings(NULL, 1, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_SUCCESS);
	Status = NtUserEnumDisplaySettings(NULL, 2, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_SUCCESS);
	Status = NtUserEnumDisplaySettings(NULL, 4, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_SUCCESS);
	Status = NtUserEnumDisplaySettings(NULL, 8, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_SUCCESS);

	Status = NtUserEnumDisplaySettings(NULL, 247, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_SUCCESS);
	Status = NtUserEnumDisplaySettings(NULL, 248, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_INVALID_PARAMETER_2);

	Status = NtUserEnumDisplaySettings(NULL, -1, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_SUCCESS);
	Status = NtUserEnumDisplaySettings(NULL, -2, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_SUCCESS);
	Status = NtUserEnumDisplaySettings(NULL, -3, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_SUCCESS);
	Status = NtUserEnumDisplaySettings(NULL, -4, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_INVALID_PARAMETER_2);

	Status = NtUserEnumDisplaySettings(&usDeviceName, ENUM_CURRENT_SETTINGS, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_INVALID_PARAMETER_1);

	usDeviceName.Buffer = szName;
	usDeviceName.Length = (USHORT)wcslen(szName);
	usDeviceName.MaximumLength = usDeviceName.Length;
	Status = NtUserEnumDisplaySettings(&usDeviceName, ENUM_CURRENT_SETTINGS, (DEVMODEW*)&data, 0);
	TEST(Status == STATUS_INVALID_PARAMETER_1);

	Status = NtUserEnumDisplaySettings(&usDeviceName, 1000, (DEVMODEW*)&data, 123456);
	TEST(Status == STATUS_INVALID_PARAMETER_1);

	TEST(GetLastError() == ERROR_SUCCESS);

}
