////////////////////////////////////////////////////////
//
// log.cpp
// 
// Script Functions
//
//
// Klemens Friedl, 19.03.2005
// frik85@hotmail.com
//
////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

#include "log.h"
#include "package.hpp"   // for Package Manager version
#include <reactos/version.h>   // ReactOS version: \reactos\include\reactos\version.h

bool LogCreated = false;

void Log (const char *message)
{
	FILE *file;
	char GTime[80];
	char version[50];
	char versionos[50];

	if (!LogCreated) {		
		file = fopen(LOGFILE, "w");
		LogCreated = true;

		// date and time
		time_t now;
		now = time(NULL);
		strftime(GTime,sizeof GTime,"%Y-%m-%d",localtime(&now));

 		// package manager version information
		wsprintfA(version, " Package Manager %d.%d.%d",
			PACKMGR_VERSION_MAJOR,
			PACKMGR_VERSION_MINOR,
			PACKMGR_VERSION_PATCH_LEVEL);

 		// operating system version information
		wsprintfA(versionos, " ReactOS %d.%d.%d",
			KERNEL_VERSION_MAJOR,
			KERNEL_VERSION_MINOR,
			KERNEL_VERSION_PATCH_LEVEL);

		fputs("# ReactOS Package Manager - Log File\n#\n# WARNING: This is still pre-alpha software.\n# Date: ", file);
		fputs(GTime, file);
		fputs("\n#\n#", file);
		fputs(version, file);
		fputs("\n#", file);
		fputs(versionos, file);
		fputs("\n#\n", file);
	}
	else		
		file = fopen(LOGFILE, "a");
		
	if (file == NULL) {

		if (LogCreated)
			LogCreated = false;

		return;
	}
	else
	{
		// Save log entry (+ add time)
		fputs("\n", file);
		time_t now;
		now = time(NULL);
		strftime(GTime,sizeof GTime,"%I:%M:%S %p  ",localtime(&now));
		fputs(GTime, file);
		fputs(message, file);
		fclose(file);
	}

	if (file)
		fclose(file);
}

void LogAdd (const char *message)
{
	FILE *file;

	file = fopen(LOGFILE, "a");
		
	// Save log entry
	fputs(message, file);
	fclose(file);

	if (file)
		fclose(file);
}
