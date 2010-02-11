#pragma once

// minimum supported platforms
// see sdkddkver.h

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#endif

// wdk
// LATEST_NTDDI_VERSION
#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x05020000
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x600
#endif

#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS _WIN32_WINNT
#endif
