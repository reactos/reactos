
ifneq ($(HOST),mingw32-windows)
  ifneq ($(HOST),mingw32-linux)
    DLLTARGET=kernel32.a
  else
    DLLTARGET=kernel32.dll
  endif
else
  DLLTARGET=kernel32.dll
endif

all: $(DLLTARGET)

SYNCH_OBJECTS = synch/critical.o synch/event.o synch/wait.o

MISC_OBJECTS = misc/error.o misc/atom.o misc/handle.o misc/env.o misc/dllmain.o \
		misc/console.o misc/time.o

FILE_OBJECTS = file/file.o file/curdir.o file/lfile.o file/dir.o \
               file/iocompl.o file/volume.o file/deviceio.o file/dosdev.o \
	       file/create.o file/find.o file/copy.o file/pipe.o \
	       file/move.o file/lock.o file/rw.o file/delete.o

MEM_OBJECTS = mem/virtual.o mem/heap.o mem/utils.o

NLS_OBJECTS =  nls/codepage.o nls/cpmisc.o nls/cptable.o\
 nls/cp37.o nls/cp437.o nls/cp500.o nls/cp737.o nls/cp775.o nls/cp850.o nls/cp852.o nls/cp855.o nls/cp857.o\
 nls/cp860.o nls/cp861.o nls/cp863.o nls/cp865.o nls/cp866.o nls/cp869.o nls/cp875.o nls/cp1026.o\
 nls/cp1250.o nls/cp1251.o nls/cp1252.o nls/cp1253.o nls/cp1254.o nls/cp1255.o nls/cp1256.o nls/cp1257.o\
 nls/cp10000.o nls/cp10006.o nls/cp10007.o nls/cp10029.o nls/cp10079.o nls/cp10081.o\
 nls/lctable.o\
 nls/lcAFK.o nls/lcBEL.o nls/lcBGR.o nls/lcCAT.o nls/lcCSY.o nls/lcDAN.o\
 nls/lcDEA.o nls/lcDEC.o nls/lcDEL.o nls/lcDES.o nls/lcDEU.o\
 nls/lcELL.o\
 nls/lcENA.o nls/lcENB.o nls/lcENC.o nls/lcENG.o nls/lcENI.o nls/lcENJ.o nls/lcENL.o nls/lcENS.o nls/lcENT.o\
 nls/lcENU.o nls/lcENZ.o\
 nls/lcESA.o nls/lcESB.o nls/lcESC.o nls/lcESD.o nls/lcESE.o nls/lcESF.o nls/lcESG.o nls/lcESH.o nls/lcESI.o\
 nls/lcESL.o nls/lcESM.o nls/lcESN.o nls/lcESO.o nls/lcESP.o nls/lcESR.o nls/lcESS.o nls/lcESU.o nls/lcESV.o\
 nls/lcESY.o nls/lcESZ.o\
 nls/lcETI.o nls/lcEUQ.o nls/lcFIN.o nls/lcFOS.o\
 nls/lcFRA.o nls/lcFRB.o nls/lcFRC.o nls/lcFRL.o nls/lcFRS.o\
 nls/lcHRV.o nls/lcHUN.o nls/lcIND.o nls/lcISL.o nls/lcITA.o nls/lcITS.o nls/lcLTH.o nls/lcLVI.o nls/lcNLB.o\
 nls/lcNLD.o nls/lcNON.o nls/lcNOR.o nls/lcPLK.o nls/lcPTB.o nls/lcPTG.o nls/lcROM.o nls/lcRUS.o nls/lcSKY.o\
 nls/lcSLV.o nls/lcSQI.o nls/lcSRB.o nls/lcSRL.o nls/lcSVE.o nls/lcSVF.o nls/lcTRK.o nls/lcUKR.o\
 nls/locale.o nls/mbtowc.o nls/wctomb.o

THREAD_OBJECTS = thread/thread.o

PROCESS_OBJECTS = process/proc.o process/cmdline.o process/create.o \
                  process/lib.o

STRING_OBJECTS = string/lstring.o

INTERNAL_OBJECTS = internal/dprintf.o internal/string.o

EXCEPT_OBJECTS = except/except.o


OBJECTS = $(MISC_OBJECTS) $(FILE_OBJECTS) $(THREAD_OBJECTS) \
          $(PROCESS_OBJECTS) $(STRING_OBJECTS) $(MEM_OBJECTS) \
	  $(INTERNAL_OBJECTS) $(SYNCH_OBJECTS) $(EXCEPT_OBJECTS)

ifeq ($(DOSCLI),yes)
CLEAN_FILES = except\*.o file\*.o internal\*.o mem\*.o misc\*.o nls\*.o  \
              process\*.o string\*.o synch\*.o thread\*.o  \
              kernel32.o kernel32.a junk.tmp base.tmp temp.exp kernel32.dll kernel32.sym
else
CLEAN_FILES = except/*.o file/*.o internal/*.o mem/*.o misc/*.o nls/*.o  \
              process/*.o string/*.o synch/*.o thread/*.o  \
              kernel32.o kernel32.a junk.tmp base.tmp temp.exp kernel32.dll kernel32.sym
endif

kernel32.a: $(OBJECTS)
	$(AR) csr kernel32.a $(OBJECTS)

kernel32.dll: $(DLLMAIN) $(OBJECTS) kernel32.def
	$(LD) -r $(OBJECTS) -o kernel32.o
	$(DLLTOOL) --dllname kernel32.dll --def kernel32.def \
	           --output-lib kernel32.a
	$(CC) -specs=k32_specs -mdll -o junk.tmp \
	      -Wl,--base-file,base.tmp kernel32.o  ../ntdll/ntdll.a
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname kernel32.dll --base-file base.tmp \
	           --output-exp temp.exp --def kernel32.def
	- $(RM) base.tmp
	$(CC) -specs=k32_specs -mdll -o kernel32.dll kernel32.o  ../ntdll/ntdll.a\
	      -Wl,--image-base,0x70000000 \
	      -Wl,--file-alignment,0x1000 \
	      -Wl,--section-alignment,0x1000 \
	      -Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort kernel32.dll > kernel32.sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

include ../../rules.mak
