
// defined exceptions used by Visual C++
// [created apennell 6/24/97]
// must be kept in sync in both V6 and V7 langapi trees

#pragma once
#if !defined(_vcexcept_h)
#define _vcexcept_h

// the facility code we have chosen is based on the fact that we already
// use an exception of 'msc' when we throw C++ exceptions

#define FACILITY_VISUALCPP  ((LONG)0x6D)

#define VcppException(sev,err)  ((sev) | (FACILITY_VISUALCPP<<16) | err)

/////////////////////////////////////////////////////////////////
// define all exceptions here, so we don't mess with each other
/////////////////////////////////////////////////////////////////

// used by CRTs for C++ exceptions, really defined in ehdata.h
//#define EH_EXCEPTION_NUMBER   VcppException( 3<<30, 0x7363 )      // SEV_ERROR, used by CRTs for C++

// used by debugger to do e.g. SetThreadName call
#define EXCEPTION_VISUALCPP_DEBUGGER    VcppException(1<<30, 5000)      // SEV_INFORMATIONAL

#endif	// _vcexcept_h
