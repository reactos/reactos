# $Id: Makefile,v 1.34 2001/04/13 16:12:24 chorns Exp $
#
# ReactOS Operating System
#

#
# Path to the directory containing the root makefile
#
PATH_TO_TOP := ..

#
# Include details of the kernel configuration
#

include config

#
#
#
CONFIG :=

ifeq ($(DBG), 1)
CFLAGS_DBG := -g
CONFIG += DBG
else
CFLAGS_DBG :=
endif

ifeq ($(KDBG), 1)
OBJECTS_KDBG := dbg/kdb.o dbg/kdb_keyboard.o dbg/i386/kdb_help.o
CONFIG += KDBG
else
OBJECTS_KDBG :=
endif

ifeq ($(MP), 1)
CONFIG += MP
else
CONFIG += UP
endif

TARGETNAME := ntoskrnl

OBJECTS_PATH = objects

ASFLAGS = -Iinclude
CFLAGS = -Iinclude -D__NTOSKRNL__ $(CFLAGS_DBG) -Wall -Werror 

include $(PATH_TO_TOP)/rules.mak

all: $(EXE_PREFIX)depends$(EXE_POSTFIX) \
	$(OBJECTS_PATH) \
	$(TARGETNAME).nostrip.exe \
	$(TARGETNAME).exe \
	$(TARGETNAME).sym

$(EXE_PREFIX)depends$(EXE_POSTFIX): depends.c
	$(HOST_CC) -o depends$(EXE_POSTFIX) depends.c

#
# Architecture specific Makefile
# Defines $(OBJECTS_ARCH)
#
include Makefile.$(ARCH)

# System API (Nt/Zw)
OBJECTS_NT = \
	nt/atom.o \
	nt/channel.o \
	nt/evtpair.o \
	nt/mutant.o \
	nt/misc.o \
	nt/nt.o \
	nt/ntevent.o \
	nt/ntsem.o \
	nt/nttimer.o \
	nt/plugplay.o \
	nt/profile.o \
	nt/zw.o \
	nt/vdm.o

# Run-Time Library (Rtl)
OBJECTS_RTL = \
	rtl/bitmap.o \
	rtl/ctype.o \
	rtl/error.o \
	rtl/interlck.o \
	rtl/largeint.o \
	rtl/mem.o \
	rtl/memchr.o \
	rtl/memcpy.o \
	rtl/memmove.o \
	rtl/memset.o \
	rtl/nls.o \
	rtl/qsort.o \
	rtl/regio.o \
	rtl/sprintf.o \
	rtl/stdlib.o \
	rtl/string.o \
	rtl/swprintf.o \
	rtl/time.o \
	rtl/unicode.o \
	rtl/wstring.o \
	rtl/bitops.o \
	rtl/memcmp.o

# Kernel (Ke)
OBJECTS_KE = \
	ke/apc.o \
	ke/bug.o \
	ke/catch.o \
	ke/critical.o \
	ke/dpc.o \
	ke/error.o \
	ke/event.o \
	ke/kqueue.o \
	ke/main.o \
	ke/mutex.o \
	ke/process.o \
	ke/sem.o \
	ke/spinlock.o \
	ke/timer.o \
	ke/wait.o \
	ke/kthread.o

# Memory Manager (Mm)
OBJECTS_MM = \
	mm/aspace.o \
	mm/cont.o \
	mm/freelist.o \
	mm/iospace.o \
	mm/marea.o \
	mm/mdl.o \
	mm/mm.o \
	mm/ncache.o \
	mm/npool.o \
	mm/pagfault.o \
	mm/pagefile.o \
	mm/paging.o \
	mm/pool.o \
	mm/ppool.o \
	mm/section.o \
	mm/virtual.o \
	mm/pager.o \
	mm/wset.o \
	mm/mminit.o \
	mm/kmap.o \
	mm/mpw.o \
	mm/pageop.o

# I/O Subsystem (Io)
OBJECTS_IO = \
	io/adapter.o \
	io/buildirp.o \
	io/cancel.o \
	io/cleanup.o \
	io/cntrller.o \
	io/create.o \
	io/device.o \
	io/dir.o \
	io/drvlck.o \
	io/errlog.o \
	io/error.o \
	io/event.o \
	io/file.o \
	io/flush.o \
	io/fs.o \
	io/iocomp.o \
	io/ioctrl.o \
	io/iomgr.o \
	io/irp.o \
	io/lock.o \
	io/mailslot.o \
	io/mdl.o \
	io/npipe.o \
	io/page.o \
	io/process.o \
	io/queue.o \
	io/resource.o \
	io/rw.o \
	io/share.o \
	io/shutdown.o \
	io/symlink.o \
	io/timer.o \
	io/vpb.o \
	io/xhaldisp.o \
	io/xhaldrv.o

# Object Manager (Ob)
OBJECTS_OB = \
	ob/dirobj.o \
	ob/handle.o \
	ob/namespc.o \
	ob/ntobj.o \
	ob/object.o \
	ob/security.o

# Process Manager (Ps)
OBJECTS_PS = \
	ps/create.o \
	ps/idle.o \
	ps/kill.o \
	ps/process.o \
	ps/psmgr.o \
	ps/thread.o \
	ps/tinfo.o \
	ps/debug.o \
	ps/suspend.o

# Executive Subsystem (Ex)
OBJECTS_EX = \
	ex/callback.o \
	ex/fmutex.o \
	ex/init.o \
	ex/interlck.o \
	ex/list.o \
	ex/locale.o \
	ex/lookas.o \
	ex/napi.o \
	ex/power.o \
	ex/resource.o \
	ex/time.o \
	ex/sysinfo.o \
	ex/work.o \
	ex/zone.o

# Installable File System Run-Time Library (FsRtl)
OBJECTS_FS = \
	fs/dbcsname.o \
	fs/filelock.o \
	fs/mcb.o \
	fs/mdl.o \
	fs/name.o \
	fs/notify.o \
	fs/oplock.o \
	fs/pool.o \
	fs/tunnel.o \
	fs/unc.o \
	fs/util.o

# Security Subsystem
OBJECTS_SE = \
	se/access.o \
	se/acl.o \
	se/luid.o \
	se/priv.o \
	se/sd.o \
	se/semgr.o \
	se/sid.o \
	se/token.o

# Configuration Manager (Registry)
OBJECTS_CM = \
	cm/registry.o

# Debugger Support (Dbg)
OBJECTS_DBG = \
	dbg/dbgctrl.o \
	dbg/errinfo.o \
	dbg/print.o \
	dbg/user.o \
	$(OBJECTS_KDBG)

# Loader
OBJECTS_LDR = \
	ldr/init.o \
	ldr/loader.o \
	ldr/resource.o \
	ldr/rtl.o \
	ldr/sysdll.o \
	ldr/userldr.o

# Local Procedure Call (Lpc)
OBJECTS_LPC = \
	lpc/close.o \
	lpc/complete.o \
	lpc/connect.o \
	lpc/create.o \
	lpc/listen.o \
	lpc/port.o \
	lpc/query.o \
	lpc/queue.o \
	lpc/receive.o \
	lpc/reply.o \
	lpc/send.o

# Nation Language Support Library (Nls)
OBJECTS_NLS = nls/nls.o

# Power Management (Po)
OBJECTS_PO = po/power.o

# Cache Manager (Cc)
OBJECTS_CC = \
	cc/cacheman.o \
	cc/view.o

# Kernel Debugger Support (Kd)
OBJECTS_KD = \
	kd/kdebug.o \
	kd/service.o \
	kd/dlog.o

C_OBJECTS := $(OBJECTS_NT) $(OBJECTS_HAL) $(OBJECTS_MM) $(OBJECTS_ARCH) \
	$(OBJECTS_IO) $(OBJECTS_KE) $(OBJECTS_OB) \
	$(OBJECTS_PS) $(OBJECTS_EX) $(OBJECTS_CC) $(OBJECTS_FS) $(OBJECTS_SE) \
	$(OBJECTS_DBG) $(OBJECTS_CM) $(OBJECTS_LDR) $(OBJECTS_LPC) \
	$(OBJECTS_PO) $(OBJECTS_KD) $(OBJECTS_RTL)

D_FILES := $(filter-out ex/napi.%, $(filter-out nt/zw.%, $(C_OBJECTS:.o=.d)))
D1_FILES := $(join $(dir $(D_FILES)), $(addprefix ., $(notdir $(D_FILES))))

# Resources
OBJECTS_RESOURCE = \
	$(TARGETNAME).coff

# IMPORTS/EXPORTS
IE_DATA = \
	$(TARGETNAME).def \
	$(TARGETNAME).edf

$(OBJECTS_PATH):
	mkdir $(OBJECTS_PATH)

#
# Build subsystem intermediate objects
#
$(OBJECTS_PATH)/hal.o: $(OBJECTS_HAL)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/hal.o \
		$(OBJECTS_HAL)

$(OBJECTS_PATH)/arch.o: $(OBJECTS_ARCH)
	$(LD) -r -o $(OBJECTS_PATH)/arch.o $(OBJECTS_ARCH)

$(OBJECTS_PATH)/io.o: $(OBJECTS_IO)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/io.o \
		$(OBJECTS_IO)

$(OBJECTS_PATH)/ke.o: $(OBJECTS_KE)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/ke.o \
		$(OBJECTS_KE) 

$(OBJECTS_PATH)/rtl.o: $(OBJECTS_RTL)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/rtl.o \
		$(OBJECTS_RTL)

$(OBJECTS_PATH)/mm.o: $(OBJECTS_MM)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/mm.o \
		$(OBJECTS_MM) 

$(OBJECTS_PATH)/ob.o: $(OBJECTS_OB)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/ob.o \
		$(OBJECTS_OB)

$(OBJECTS_PATH)/ps.o: $(OBJECTS_PS)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/ps.o \
		$(OBJECTS_PS)

$(OBJECTS_PATH)/ex.o: $(OBJECTS_EX)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/ex.o \
		$(OBJECTS_EX)

$(OBJECTS_PATH)/fs.o: $(OBJECTS_FS)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/fs.o \
		$(OBJECTS_FS)

$(OBJECTS_PATH)/se.o: $(OBJECTS_SE)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/se.o \
		$(OBJECTS_SE)

$(OBJECTS_PATH)/cm.o: $(OBJECTS_CM)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/cm.o \
		$(OBJECTS_CM)

$(OBJECTS_PATH)/dbg.o: $(OBJECTS_DBG)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/dbg.o \
		$(OBJECTS_DBG)

$(OBJECTS_PATH)/ldr.o: $(OBJECTS_LDR)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/ldr.o \
		$(OBJECTS_LDR)

$(OBJECTS_PATH)/lpc.o: $(OBJECTS_LPC)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/lpc.o \
		$(OBJECTS_LPC)

$(OBJECTS_PATH)/nls.o: $(OBJECTS_NLS)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/nls.o \
		$(OBJECTS_NLS)

$(OBJECTS_PATH)/nt.o: $(OBJECTS_NT)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/nt.o \
		$(OBJECTS_NT)

$(OBJECTS_PATH)/po.o: $(OBJECTS_PO)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/po.o \
		$(OBJECTS_PO)

$(OBJECTS_PATH)/cc.o: $(OBJECTS_CC)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/cc.o \
		$(OBJECTS_CC) 

$(OBJECTS_PATH)/kd.o: $(OBJECTS_KD)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/kd.o \
		$(OBJECTS_KD) 

$(TARGETNAME).coff: $(TARGETNAME).rc ../include/reactos/resource.h


# Note: arch.o MUST be the first file!!!
OBJECTS := \
	$(OBJECTS_PATH)/arch.o \
	$(OBJECTS_PATH)/ke.o \
	$(OBJECTS_PATH)/hal.o \
	$(OBJECTS_PATH)/cc.o \
	$(OBJECTS_PATH)/cm.o \
	$(OBJECTS_PATH)/dbg.o \
	$(OBJECTS_PATH)/ex.o \
	$(OBJECTS_PATH)/lpc.o \
	$(OBJECTS_PATH)/fs.o \
	$(OBJECTS_PATH)/io.o \
	$(OBJECTS_PATH)/kd.o \
	$(OBJECTS_PATH)/ldr.o \
	$(OBJECTS_PATH)/mm.o \
	$(OBJECTS_PATH)/nls.o \
	$(OBJECTS_PATH)/nt.o \
	$(OBJECTS_PATH)/ob.o \
	$(OBJECTS_PATH)/po.o \
	$(OBJECTS_PATH)/ps.o \
	$(OBJECTS_PATH)/rtl.o \
	$(OBJECTS_PATH)/se.o \
	$(OBJECTS_RESOURCE)


ifeq ($(DOSCLI),yes)
CLEAN_FILES = $(OBJECTS_PATH)\*.o cc\*.o cm\*.o dbg\*.o ex\*.o hal\x86\*.o io\*.o \
              ke\*.o ldr\*.o mm\*.o nt\*.o ob\*.o ps\*.o rtl\*.o se\*.o \
              ke\i386\*.o mm\i386\*.o fs\*.o po\*.o nls\*.o lpc\*.o \
              kd\*.o $(TARGETNAME).o $(TARGETNAME).a junk.tmp base.tmp temp.exp \
              $(TARGETNAME).exe $(TARGETNAME).nostrip.exe $(TARGETNAME).sym $(TARGETNAME).coff $(D1_FILES)
else
CLEAN_FILES = $(OBJECTS_PATH)/*.o cc/*.o cm/*.o dbg/*.o ex/*.o hal/x86/*.o io/*.o \
              ke/*.o ldr/*.o mm/*.o nt/*.o ob/*.o ps/*.o rtl/*.o se/*.o \
              ke/i386/*.o mm/i386/*.o fs/*.o po/*.o nls/*.o lpc/*.o \
              kd/*.o $(TARGETNAME).o $(TARGETNAME).a junk.tmp base.tmp temp.exp \
              $(TARGETNAME).exe $(TARGETNAME).nostrip.exe $(TARGETNAME).sym $(TARGETNAME).coff $(D1_FILES)
endif


$(TARGETNAME).nostrip.exe: $(TARGETNAME).o $(TARGETNAME).a $(IE_DATA)
	$(CC) \
		-Wl,-T,ntoskrnl.lnk \
		-nostartfiles -nostdlib \
		-mdll \
		-o junk.tmp \
		-Wl,--subsystem,native \
		-Wl,--image-base,0xc0000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--base-file,base.tmp \
		$(TARGETNAME).o -lgcc
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).exe \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGETNAME).edf \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		-nostartfiles -nostdlib \
		-Wl,-T,ntoskrnl.lnk \
		-mdll \
		-o $(TARGETNAME).nostrip.exe \
		-Wl,--subsystem,native \
		-Wl,--image-base,0xc0000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp \
		$(TARGETNAME).o -lgcc
	- $(RM) temp.exp

$(TARGETNAME).exe: $(TARGETNAME).o $(TARGETNAME).a $(TARGETNAME).lnk
	$(CC) \
		-Wl,-T,ntoskrnl.lnk \
		-nostartfiles -nostdlib \
		-mdll \
		-o junk.tmp \
		-Wl,--subsystem,native \
		-Wl,--image-base,0xc0000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--base-file,base.tmp \
		$(TARGETNAME).o -lgcc
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).exe \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGETNAME).edf \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		-Wl,-T,ntoskrnl.lnk -Wl,-s\
		-nostartfiles -nostdlib \
		-mdll \
		-o $(TARGETNAME).exe \
		-Wl,--subsystem,native \
		-Wl,--image-base,0xc0000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp \
		$(TARGETNAME).o -lgcc
	- $(RM) temp.exp

$(TARGETNAME).sym: $(TARGETNAME).nostrip.exe
	$(NM) --numeric-sort $(TARGETNAME).nostrip.exe > $(TARGETNAME).sym

$(TARGETNAME).o: $(OBJECTS)
	$(LD) \
		-r \
		-o $(TARGETNAME).o \
		$(OBJECTS)

$(TARGETNAME).a: $(TARGETNAME).def
	$(DLLTOOL) \
		--dllname $(TARGETNAME).exe \
		--def $(TARGETNAME).def \
		--output-lib $(TARGETNAME).a \
		--kill-at

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

install: $(FLOPPY_DIR)/$(TARGETNAME).exe

$(FLOPPY_DIR)/$(TARGETNAME).exe: $(TARGETNAME).exe
	$(CP) $(TARGETNAME).exe $(FLOPPY_DIR)/$(TARGETNAME).exe

.PHONY: install

dist: ../$(DIST_DIR)/$(TARGETNAME).exe

../$(DIST_DIR)/$(TARGETNAME).exe: $(TARGETNAME).exe
	$(CP) $(TARGETNAME).exe ../$(DIST_DIR)/$(TARGETNAME).exe

.PHONY: dist

ex/napi.o: ex/napi.c ../include/ntdll/napi.h

ke/main.o: ke/main.c ../include/reactos/buildno.h

mkconfig$(EXE_SUFFIX): mkconfig.c
	$(HOST_CC) -g -o mkconfig$(EXE_SUFFIX) mkconfig.c

config:
	$(EXE_PREFIX)mkconfig$(EXE_SUFFIX) include/internal/config.h $(CONFIG)

include/internal/config.h: config mkconfig$(EXE_SUFFIX)
	$(EXE_PREFIX)mkconfig$(EXE_SUFFIX) include/internal/config.h$(CONFIG)

ifneq ($(MAKECMDGOALS),clean)
include $(D1_FILES)
endif

.%.d: %.c $(EXE_PREFIX)depends$(EXE_POSTFIX)
	$(CC) $(CFLAGS) -M $< | $(EXE_PREFIX)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.s
	$(CC) $(CFLAGS) -M $< | $(EXE_PREFIX)depends$(EXE_POSTFIX) $(@D) $@ 

.%.d: %.S
	$(CC) $(CFLAGS) -M $< | $(EXE_PREFIX)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.asm
	$(NASM_CMD) $< | $(EXE_PREFIX)depends$(EXE_POSTFIX) $(@D) $@

# EOF
