////////////////////////////////////////////////////////
//
// log.h
//
// Script Functions
//
//
// Klemens Friedl, 19.03.2005
// frik85@hotmail.com
//
////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>

#define LOGFILE	"logfile.html"

extern bool LogCreated;

void Log (const char *message);
void LogAdd (const char *message);
