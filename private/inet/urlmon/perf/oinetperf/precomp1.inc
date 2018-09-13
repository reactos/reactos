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
#PRECOMPILED_INCLUDE=..\..\inc\urlb.h
#PRECOMPILED_TARGET=$(GPCH_BUILD)\$(_OBJ_DIR)\*\urlb.pch
#PRECOMPILED_OPTION=/Yuurlb.h /Fp$(GPCH_BUILD)\$(_OBJ_DIR)\*\urlb.pch
#PRECOMPILED_OBJ=$(GPCH_BUILD)\$(_OBJ_DIR)\*\urlb.obj

