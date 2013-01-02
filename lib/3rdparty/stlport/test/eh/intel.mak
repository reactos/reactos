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
F90=fl32.exe

OUTDIR=.
INTDIR=.

# set this directories 
STL_INCL=../../stlport
VC_INCL=.
# d:/vc41/msdev/include

Dep_stl = TestClass.obj main.obj nc_alloc.obj \
random_number.obj test_algo.obj test_algobase.obj test_bit_vector.obj test_deque.obj \
test_hash_map.obj test_hash_set.obj test_list.obj test_map.obj test_rope.obj test_set.obj \
test_slist.obj test_vector.obj test_string.obj test_bitset.obj test_valarray.obj

LINK32=link.exe

CPP_PROJ=/nologo /W3 /GX /D "WIN32" /MTd /Zi /Gm /Od /D "_CONSOLE"   /I$(STL_INCL) /I. /D_DEBUG

CPP_LIBS = /link /libpath:"..\..\lib"

check: eh_test.out

eh_test.out : $(Dep_stl)
	$(CPP) $(CPP_PROJ) $(Dep_stl) -o eh_test.exe $(CPP_LIBS)
	.\eh_test.exe
	echo done

clean :
	-@erase "$(INTDIR)\*.obj"
	-@erase "$(OUTDIR)\*.exe"
	-@erase "$(OUTDIR)\*.obj"


.exe.out:
	$< > $@

.cpp.exe:
  $(CPP) $(CPP_PROJ) -DMAIN $< $(CPP_LIBS)

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
