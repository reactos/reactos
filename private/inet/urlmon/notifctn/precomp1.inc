#//+---------------------------------------------------------------
#//
#//  File:      precomp1.inc
#//
#//  Contents:  directives for global precompiled include file when the
#//             sources file is two directories below com (or wherever
#//             this file is located).  We could easily have precomp3 and
#//             precomp4 for other areas.The urlint.* files should not
#//             otherwise be mentioned in the sources files.
#//
#//
#//----------------------------------------------------------------

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\..\inc\notiftn.h
PRECOMPILED_TARGET=$(GPCH_BUILD)\$(_OBJ_DIR)\*\notiftn.pch
PRECOMPILED_OPTION=/Yunotiftn.h /Fp$(GPCH_BUILD)\$(_OBJ_DIR)\*\notiftn.pch
PRECOMPILED_OBJ=$(GPCH_BUILD)\$(_OBJ_DIR)\*\notiftn.obj

