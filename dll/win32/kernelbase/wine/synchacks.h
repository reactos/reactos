#pragma once

#define CRITICAL_SECTION RTL_CRITICAL_SECTION
#define CRITICAL_SECTION_DEBUG RTL_CRITICAL_SECTION_DEBUG

#ifndef RtlReleasePath
#define RtlReleasePath( path ) RtlFreeHeap( GetProcessHeap(), 0, path );
#endif
