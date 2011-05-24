# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103


!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
RSC=rc.exe
CPP=icl.exe
LINK32=xilink.exe

OUTDIR=.
INTDIR=.

# set this directories 
STL_PATH=..\..

Dep_stl = TestClass.obj main.obj nc_alloc.obj \
random_number.obj test_algo.obj test_algobase.obj test_bit_vector.obj test_deque.obj \
test_hash_map.obj test_hash_set.obj test_list.obj test_map.obj test_rope.obj test_set.obj \
test_slist.obj test_vector.obj test_string.obj test_bitset.obj test_valarray.obj

CPP_LIBS = /link /incremental:no /LIBPATH:$(STL_PATH)\lib

#disable warnings complaining about debug ...info exceeded....
CPP_PRJ_EXTRA = /Qwd985
CPP_PRJ_CMN = /nologo /W3 /GR /GX /DWIN32 /D_WINDOWS /D_CONSOLE /I$(STL_PATH)\stlport /I.

#
LIBTYPE = STATIC
# LIBTYPE = DYNAMIC
#
#DEBUG = STL
DEBUG = ON
#DEBUG =
# 
IOS = SGI
#IOS = NOSGI
#IOS = NONE

!IF "$(IOS)" == "NOSGI"
CPP_PRJ_IOS = /D_STLP_NO_OWN_IOSTREAMS
!ELSEIF "$(IOS)" == "NONE"
CPP_PRJ_IOS = /D_STLP_NO_IOSTREAM
!ELSE
CPP_PRJ_IOS =
!ENDIF

#MT/MD etc should be LAST in CPP_PRJ_LIBTYP string!!!
#Library selection should be BEFORE debug processing!!!
!IF "$(LIBTYPE)" == "STATIC"
CPP_PRJ_LIBTYP = /MT
!ELSE
CPP_PRJ_LIBTYP = /MD
!ENDIF

!IF "$(DEBUG)" == ""
CPP_PRJ_DBG = /DNDEBUG /O2 /Qsox-
!ELSE
CPP_PRJ_LIBTYP = $(CPP_PRJ_LIBTYP)d
CPP_PRJ_DBG = /D_DEBUG /Od
!IF "$(DEBUG)" == "STL"
CPP_PRJ_DBG = $(CPP_PRJ_DBG) /D_STLP_DEBUG
!ENDIF
CPP_PRJ_CMN = $(CPP_PRJ_CMN) /Zi /Gm
!ENDIF

CPP_PROJ = $(CPP_PRJ_CMN) $(CPP_PRJ_EXTRA) $(CPP_PRJ_IOS) $(CPP_PRJ_LIBTYP) $(CPP_PRJ_DBG)

check: eh_test.out

eh_test.out : $(Dep_stl)
	$(CPP) $(CPP_PROJ) $(Dep_stl) /Feeh_test.exe $(CPP_LIBS)
        cd ..\..\lib
	..\test\eh\eh_test.exe -s 100
	echo done

clean :
	-@erase "$(INTDIR)\*.obj"
	-@erase "$(OUTDIR)\*.exe"
	-@erase "$(OUTDIR)\*.obj"


.exe.out:
	$< > $@

.cpp.exe:
  $(CPP) $(CPP_PROJ) -DMAIN $<

.c.obj:
   $(CPP) $(CPP_PROJ) /c $<

.cpp.obj:
   $(CPP) $(CPP_PROJ) /c $<

.cxx.obj:
   $(CPP) $(CPP_PROJ) /c $<

.cpp.E:
   $(CPP) $(CPP_PROJ) -E $< >$*.E  

.cpp.sbr:
   $(CPP) $(CPP_PROJ) $<  
