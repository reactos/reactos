extern "C" {
#include <nt.h>
#include <ntioapi.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <ddeml.h>      // for CP_WINUNICODE

#include <objidl.h>
#include <propidl.h>

#ifdef _CAIRO_
#include <iofs.h>
#else
extern "C"
{
#include <propapi.h>
}
#endif

#include <stgprop.h>

#include <stgvar.hxx>
#include <propstm.hxx>
#ifdef _CAIRO_
#include <memser.hxx>
#include <memdeser.hxx>
#endif
#include <align.hxx>
#include <sstream.hxx>

#include "propmac.hxx"
#pragma hdrstop
