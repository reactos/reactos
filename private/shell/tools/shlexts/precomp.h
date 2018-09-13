#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <excpt.h>
#include <stdio.h>
#include <limits.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <string.h>
#include <ntstatus.h>
#include <windows.h>
#include <ntddvdeo.h>

#include <shellapi.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <commdlg.h>

#ifndef FIELDOFFSET
#define FIELDOFFSET(type, field)    ((int)(&((type NEAR*)1)->field)-1)
#endif
