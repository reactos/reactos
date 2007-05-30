#include "Log.h"
#include <stdarg.h>
#include <stdio.h>
#pragma hdrstop	

BOOLEAN LogMessage(PCHAR szFormat, ...)
{
	ULONG Length;
	char messagebuf[256];
	va_list va;
    IO_STATUS_BLOCK  IoStatus;
	OBJECT_ATTRIBUTES objectAttributes;
	NTSTATUS status;
	HANDLE FileHandle;
    UNICODE_STRING fileName;

		
	//format the string
    va_start(va,szFormat);
	vsprintf(messagebuf,szFormat,va);
	va_end(va);

	//get a handle to the log file object
    fileName.Buffer = NULL;
    fileName.Length = 0;
    fileName.MaximumLength = sizeof(DEFAULT_LOG_FILE_NAME) + sizeof(UNICODE_NULL);
    fileName.Buffer = ExAllocatePool(PagedPool,
                                        fileName.MaximumLength);
    if (!fileName.Buffer)
    {
        return FALSE;
    }
    RtlZeroMemory(fileName.Buffer, fileName.MaximumLength);
    status = RtlAppendUnicodeToString(&fileName, (PWSTR)DEFAULT_LOG_FILE_NAME);
	
	//DbgPrint("\n Initializing Object attributes");

	InitializeObjectAttributes (&objectAttributes,
								(PUNICODE_STRING)&fileName,
								OBJ_CASE_INSENSITIVE,
								NULL,
								NULL );

	DbgPrint("\n BusLogic - Creating the file");
	
	status = ZwCreateFile(&FileHandle,
					  FILE_APPEND_DATA,
					  &objectAttributes,
					  &IoStatus,
					  0, 
					  FILE_ATTRIBUTE_NORMAL,
					  FILE_SHARE_WRITE,
					  FILE_OPEN_IF,
					  FILE_SYNCHRONOUS_IO_NONALERT,
					  NULL,     
					  0 );

	if(NT_SUCCESS(status))
	{
		CHAR buf[300];
		LARGE_INTEGER time;
		KeQuerySystemTime(&time);

		DbgPrint("\n BusLogic - Created the file");

		//put a time stamp on the output message
		sprintf(buf,"%10u-%10u  %s",time.HighPart,time.LowPart,messagebuf);

		//format the string to make sure it appends a newline carrage-return to the 
		//end of the string.
		Length=strlen(buf);
		if(buf[Length-1]=='\n')
		{
			buf[Length-1]='\r';
			strcat(buf,"\n");
			Length++;
		}
		else
		{
			strcat(buf,"\r\n");
			Length+=2;
		}

		buf[Length+1] = '\0';
		DbgPrint("\n BusLogic - Writing to the file");
		DbgPrint("\n BusLogic - Buf = %s", buf);
		
		status = ZwWriteFile(FileHandle,
				  NULL,
				  NULL,
				  NULL,
				  &IoStatus,
				  buf,
				  Length,
				  NULL,
				  NULL );

		ZwClose(FileHandle);
	}
	if (fileName.Buffer)
        ExFreePool (fileName.Buffer);

	return STATUS_SUCCESS;
}
