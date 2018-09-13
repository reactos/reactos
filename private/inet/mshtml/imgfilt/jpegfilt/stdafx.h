// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if DBG==1
#define _DEBUG
#else
#define _ATL_MIN_CRT
#endif

#include "atlbase.h"
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include "atlcom.h"

#include "ImgUtil.H"

extern "C" {
#include "JPEGLib\jpeglib.h"
#include "JPEGLib\jerror.h"
}
