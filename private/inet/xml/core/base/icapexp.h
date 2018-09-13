/*****************************************************************************/
/*                                                                           */
/*    ICAPEXP.H -- Exports from ICAP.DLL				     */
/*									     */
/*    Copyright (C) 1995 by Microsoft Corp.				     */
/*    All rights reserved						     */
/*									     */
/*****************************************************************************/

#ifndef __ICAPEXP_H__
#define __ICAPEXP_H__

#ifndef PROFILE
#define PROFILE 1		// define this as zero to macro-out the API
#endif

#if PROFILE

#ifdef __cplusplus
extern "C"
{
#endif

int __stdcall StartCAP(void);	// start profiling
int __stdcall StopCAP(void);    // stop profiling until StartCAP
int __stdcall SuspendCAP(void); // suspend profiling until ResumeCAP
int __stdcall ResumeCAP(void);  // resume profiling

int __stdcall StartCAPAll(void);    // process-wide start profiling
int __stdcall StopCAPAll(void);     // process-wide stop profiling
int __stdcall SuspendCAPAll(void);  // process-wide suspend profiling
int __stdcall ResumeCAPAll(void);   // process-wide resume profiling

void __stdcall MarkCAP(long lMark);  // write mark to MEA

void __stdcall AllowCAP(void);  // Allow profiling when 'profile=almostnever'


#ifdef __cplusplus
}
#endif

#else // NOT PROFILE

#define StartCAP()      0
#define StopCAP()       0
#define SuspendCAP()    0
#define ResumeCAP()     0

#define StartCAPAll()   0
#define StopCAPAll()    0
#define SuspendCAPAll() 0
#define ResumeCAPAll()  0

#define MarkCAP(n)      0

#define AllowCAP()      0

#endif // NOT PROFILE

#endif  // __ICAPEXP_H__
