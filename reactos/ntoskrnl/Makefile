# $Id: Makefile,v 1.128 2004/06/23 22:32:45 ion Exp $
#
# ReactOS Operating System
#

#
# Path to the directory containing the root makefile
#
PATH_TO_TOP := ..

#
# Include details of the host OS configuration
#
include $(PATH_TO_TOP)/config

CONFIG :=

ifeq ($(DBG), 1)
CFLAGS_DBG := -g
LINKER_SCRIPT := ntoskrnl.lnk
STRIP_FLAGS := -Wl,-s
else
CFLAGS_DBG :=
LINKER_SCRIPT := ntoskrnl.lnk
STRIP_FLAGS := -Wl,-s
endif

ifeq ($(KDBG), 1)
OBJECTS_KDBG := dbg/kdb.o dbg/kdb_serial.o dbg/kdb_keyboard.o dbg/rdebug.o \
                dbg/i386/kdb_help.o dbg/kdb_stabs.o dbg/profile.o \
		../dk/w32/lib/kjs.a dbg/i386/i386-dis.o
CFLAGS += -I../lib/kjs/include
preall: all

../dk/w32/lib/kjs.a:
	$(MAKE) -C ../lib/kjs
else
OBJECTS_KDBG :=
endif

ifeq ($(strip $(SDK_PATH_INC)),)
ASFLAGS += -I./include
CFLAGS += -I./include -D__NTOSKRNL__ $(CFLAGS_DBG) -Wall -Werror
else
ASFLAGS += -I./include -I$(SDK_PATH_INC)
CFLAGS += -I./include -I$(SDK_PATH_INC) -D__NTOSKRNL__ $(CFLAGS_DBG) -Wall -Werror
endif

# ASFLAGS += -D__USE_W32API -D_NTOSKRNL_
# CFLAGS += -D__USE_W32API -D_NTOSKRNL_

# require os code to explicitly request A/W version of structs/functions
CFLAGS += -D_DISABLE_TIDENTS

# no native setjmp/longjmp in the kernel
CFLAGS += -D_SEH_NO_NATIVE_NLG

CFLAGS += $(STD_CFLAGS)
ASFLAGS += $(STD_ASFLAGS)
RCFLAGS += $(STD_RCFLAGS)

#
# Build configuration
#
include $(PATH_TO_TOP)/rules.mak

#
# Javascript extension for kdb
#

TARGETNAME := ntoskrnl

OBJECTS_PATH = objects

all: $(OBJECTS_PATH) \
	bugcodes.rc $(PATH_TO_TOP)/include/reactos/bugcodes.h \
	$(TARGETNAME).nostrip.exe \
	$(TARGETNAME).exe \
	$(TARGETNAME).sym \
	ntoskrnl.map

#
# Architecture specific Makefile
# Defines $(OBJECTS_ARCH)
#
include Makefile.$(ARCH)

# System API (Nt/Zw)
OBJECTS_NT = \
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
	nt/vdm.o \
	nt/zw.o

# Run-Time Library (Rtl)
OBJECTS_RTL = \
	rtl/atom.o \
	rtl/bit.o \
	rtl/bitmap.o \
	rtl/capture.o \
	rtl/ctype.o \
	rtl/handle.o \
	rtl/message.o \
	rtl/purecall.o \
	rtl/regio.o \
	rtl/sprintf.o \
	rtl/stdlib.o \
	rtl/string.o \
	rtl/swprintf.o \
	rtl/wstring.o \
	rtl/nls.o \
	rtl/rangelist.o \
	rtl/libsupp.o

OBJECTS_RTL := $(filter-out $(RTL_EXCLUDE_FILTER), $(OBJECTS_RTL))


# Kernel (Ke)
OBJECTS_KE = \
	ke/apc.o \
	ke/bug.o \
	ke/catch.o \
	ke/critical.o \
	ke/dpc.o \
        ke/device.o \
	ke/error.o \
	ke/event.o \
	ke/kqueue.o \
	ke/kthread.o \
	ke/main.o \
	ke/mutex.o \
	ke/process.o \
        ke/profile.o \
	ke/queue.o \
	ke/sem.o \
	ke/spinlock.o \
	ke/timer.o \
	ke/wait.o

# Memory Manager (Mm)
OBJECTS_MM = \
	mm/anonmem.o \
	mm/aspace.o \
	mm/balance.o \
	mm/cont.o \
	mm/drvlck.o \
	mm/freelist.o \
	mm/iospace.o \
	mm/kmap.o \
	mm/marea.o \
	mm/mdl.o \
	mm/mm.o \
	mm/mminit.o \
	mm/mpw.o \
	mm/ncache.o \
	mm/npool.o \
	mm/pagfault.o \
	mm/pagefile.o \
	mm/pageop.o \
	mm/pager.o \
	mm/paging.o \
	mm/pool.o \
	mm/ppool.o \
	mm/region.o \
	mm/rmap.o \
	mm/section.o \
	mm/slab.o \
	mm/virtual.o \
	mm/wset.o

# I/O Subsystem (Io)
OBJECTS_IO = \
	io/adapter.o \
	io/arcname.o \
	io/buildirp.o \
	io/cancel.o \
	io/cleanup.o \
	io/cntrller.o \
	io/create.o \
	io/device.o \
	io/deviface.o \
	io/dir.o \
	io/driver.o \
	io/errlog.o \
	io/error.o \
	io/event.o \
	io/file.o \
	io/flush.o \
	io/fs.o \
	io/iocomp.o \
	io/ioctrl.o \
	io/iomgr.o \
	io/iowork.o \
	io/irp.o \
	io/lock.o \
	io/mailslot.o \
	io/mdl.o \
	io/npipe.o \
	io/page.o \
	io/parttab.o \
	io/process.o \
	io/pnpnotify.o \
	io/pnpdma.o \
	io/pnpmgr.o \
	io/pnpreport.o \
	io/pnproot.o \
	io/queue.o \
	io/rawfs.o \
	io/remlock.o \
	io/resource.o \
	io/rw.o \
	io/share.o \
	io/shutdown.o \
	io/symlink.o \
	io/timer.o \
	io/vpb.o \
	io/wdm.o \
	io/xhaldisp.o \
        io/xhaldrv.o \
        io/wmi.o

# Object Manager (Ob)
OBJECTS_OB = \
	ob/dirobj.o \
	ob/handle.o \
	ob/namespc.o \
	ob/ntobj.o \
	ob/object.o \
	ob/security.o \
	ob/symlink.o

# Process Manager (Ps)
OBJECTS_PS = \
	ps/create.o \
	ps/debug.o \
	ps/idle.o \
	ps/kill.o \
	ps/locale.o \
	ps/process.o \
	ps/psmgr.o \
	ps/suspend.o \
	ps/thread.o \
	ps/tinfo.o \
	ps/win32.o \
	ps/w32call.o

# Executive Subsystem (Ex)
OBJECTS_EX = \
	ex/btree.o \
	ex/callback.o \
	ex/fmutex.o \
	ex/hashtab.o \
	ex/init.o \
	ex/interlck.o \
	ex/list.o \
	ex/lookas.o \
	ex/napi.o \
	ex/power.o \
	ex/resource.o \
        ex/rundown.o \
	ex/stree.o \
        ex/synch.o \
	ex/sysinfo.o \
        ex/time.o \
        ex/util.o \
	ex/win32k.o \
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
	se/audit.o \
	se/lsa.o \
	se/luid.o \
	se/priv.o \
	se/sd.o \
	se/semgr.o \
	se/sid.o \
	se/token.o

# Configuration Manager (Registry)
OBJECTS_CM = \
	cm/import.o  \
	cm/ntfunc.o \
	cm/regfile.o \
	cm/registry.o  \
	cm/regobj.o \
	cm/rtlfunc.o

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

# Power Management (Po)
OBJECTS_PO = \
	po/power.o

# Cache Manager (Cc)
OBJECTS_CC = \
	cc/cacheman.o \
	cc/copy.o \
	cc/fs.o \
	cc/pin.o \
	cc/mdl.o \
	cc/misc.o \
	cc/view.o

# Kernel Debugger Support (Kd)
OBJECTS_KD = \
	kd/dlog.o \
    kd/gdbstub.o \
	kd/kdebug.o \
    kd/mda.o \
	kd/service.o

# Boot video (Inbv)
OBJECTS_INBV = \
	inbv/inbv.o

DEP_OBJECTS := $(OBJECTS_NT) $(OBJECTS_MM) $(OBJECTS_ARCH) \
	$(OBJECTS_IO) $(OBJECTS_KE) $(OBJECTS_OB) \
	$(OBJECTS_PS) $(OBJECTS_EX) $(OBJECTS_CC) $(OBJECTS_FS) $(OBJECTS_SE) \
	$(OBJECTS_DBG) $(OBJECTS_CM) $(OBJECTS_LDR) $(OBJECTS_LPC) \
	$(OBJECTS_PO) $(OBJECTS_KD) $(OBJECTS_RTL)

TAG_OBJECTS := $(join $(dir $(DEP_OBJECTS)),$(patsubst %.o, .%.TAG, $(notdir $(DEP_OBJECTS))))

#
# Include automatic dependancy tracking
#
DEP_EXCLUDE_FILTER := ex/napi.% nt/zw.%
GENERATED_HEADER_FILES := $(PATH_TO_TOP)/include/reactos/bugcodes.h
include $(PATH_TO_TOP)/tools/depend.mk


# Resources
OBJECTS_RESOURCE = \
	$(TARGETNAME).coff

# IMPORTS/EXPORTS
IE_DATA = \
	$(TARGETNAME).def \
	$(TARGETNAME).edf

$(OBJECTS_PATH):
	mkdir $(OBJECTS_PATH)

$(OBJECTS_RESOURCE): $(TARGETNAME).rc bugcodes.rc $(PATH_TO_TOP)/include/reactos/resource.h

#
# Build subsystem intermediate objects
#
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

$(OBJECTS_PATH)/inbv.o: $(OBJECTS_INBV)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/inbv.o \
		$(OBJECTS_INBV)

# Note: arch.o MUST be the first file!!!
OBJECTS := \
	$(OBJECTS_PATH)/arch.o \
	$(OBJECTS_PATH)/ke.o \
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
	$(OBJECTS_PATH)/nt.o \
	$(OBJECTS_PATH)/ob.o \
	$(OBJECTS_PATH)/po.o \
	$(OBJECTS_PATH)/ps.o \
	$(OBJECTS_PATH)/rtl.o \
	$(OBJECTS_PATH)/se.o \
	$(OBJECTS_PATH)/inbv.o \
	$(OBJECTS_RESOURCE)


$(TARGETNAME).nostrip.exe: $(TARGETNAME).o $(IE_DATA) $(SDK_PATH_LIB)/rtl.a $(SDK_PATH_LIB)/string.a $(SDK_PATH_LIB)/rosrtl.a
	$(CC) \
		-Wl,-T,ntoskrnl.lnk \
		-nostartfiles \
		-nostdlib \
		-mdll \
		-o junk.tmp \
		-Wl,--subsystem,native \
		-Wl,--image-base,0xc0000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--entry,_NtProcessStartup \
		-Wl,--base-file,base.tmp \
		$(TARGETNAME).o -lgcc \
		$(SDK_PATH_LIB)/rtl.a \
		$(SDK_PATH_LIB)/string.a \
		$(SDK_PATH_LIB)/rosrtl.a \
		$(DDK_PATH_LIB)/hal.a
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).exe \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGETNAME).edf \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		-nostartfiles \
		-nostdlib \
		-Wl,-T,ntoskrnl.lnk \
		-mdll \
		-o $(TARGETNAME).nostrip.exe \
		-Wl,--subsystem,native \
		-Wl,--image-base,0xc0000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--entry,_NtProcessStartup \
		-Wl,temp.exp \
		$(TARGETNAME).o -lgcc \
		$(SDK_PATH_LIB)/rtl.a \
		$(SDK_PATH_LIB)/string.a \
		$(SDK_PATH_LIB)/rosrtl.a \
		$(DDK_PATH_LIB)/hal.a
	- $(RM) temp.exp

$(TARGETNAME).exe: $(TARGETNAME).o $(LINKER_SCRIPT) $(DDK_PATH_LIB)/hal.a $(SDK_PATH_LIB)/rtl.a $(SDK_PATH_LIB)/string.a $(SDK_PATH_LIB)/rosrtl.a
	- $(CC) \
		-Wl,-T,$(LINKER_SCRIPT) \
		-nostartfiles \
		-nostdlib \
		-mdll \
		-o junk.tmp \
		-Wl,--subsystem,native \
		-Wl,--image-base,0xc0000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--entry,_NtProcessStartup \
		-Wl,--base-file,base.tmp \
		$(TARGETNAME).o -lgcc \
		$(SDK_PATH_LIB)/rtl.a \
		$(SDK_PATH_LIB)/string.a \
		$(SDK_PATH_LIB)/rosrtl.a \
		$(DDK_PATH_LIB)/hal.a
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).exe \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGETNAME).edf \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		-Wl,-T,$(LINKER_SCRIPT) $(STRIP_FLAGS)\
		-nostartfiles \
		-nostdlib \
		-mdll \
		-o $(TARGETNAME).exe \
		-Wl,--subsystem,native \
		-Wl,--image-base,0xc0000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--entry,_NtProcessStartup \
		-Wl,temp.exp \
		$(TARGETNAME).o -lgcc \
		$(SDK_PATH_LIB)/rtl.a \
		$(SDK_PATH_LIB)/string.a \
		$(SDK_PATH_LIB)/rosrtl.a \
		$(DDK_PATH_LIB)/hal.a
	- $(RM) temp.exp

$(TARGETNAME).dbg.o: $(TARGETNAME).o
	$(STRIP) --strip-debug -o $(TARGETNAME).dbg.o $(TARGETNAME).o

$(TARGETNAME).dbg: $(TARGETNAME).dbg.o $(TARGETNAME).a $(TARGETNAME).dbg.lnk $(DDK_PATH_LIB)/hal.a $(SDK_PATH_LIB)/rtl.a $(SDK_PATH_LIB)/string.a $(SDK_PATH_LIB)/rosrtl.a
	$(CC) \
		-Wl,-T,$(TARGETNAME).dbg.lnk \
		-nostartfiles \
		-nostdlib \
		-mdll \
		-o junk.tmp \
		-Wl,--subsystem,native \
		-Wl,--image-base,0xc0000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--entry,_NtProcessStartup \
		-Wl,--base-file,base.tmp \
		$(TARGETNAME).dbg.o -lgcc \
		$(SDK_PATH_LIB)/rtl.a \
		$(SDK_PATH_LIB)/string.a \
		$(SDK_PATH_LIB)/rosrtl.a \
		$(DDK_PATH_LIB)/hal.a
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).dbg \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGETNAME).edf \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		-Wl,-T,$(TARGETNAME).dbg.lnk \
		-nostartfiles \
		-nostdlib \
		-mdll \
		-o $(TARGETNAME).dbg \
		-Wl,--subsystem,native \
		-Wl,--image-base,0xc0000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--entry,_NtProcessStartup \
		-Wl,temp.exp \
		$(TARGETNAME).dbg.o -lgcc \
		$(SDK_PATH_LIB)/rtl.a \
		$(SDK_PATH_LIB)/string.a \
		$(SDK_PATH_LIB)/rosrtl.a \
		$(DDK_PATH_LIB)/hal.a
	- $(RM) temp.exp

$(TARGETNAME).sym: $(TARGETNAME).nostrip.exe
	$(RSYM) $(TARGETNAME).nostrip.exe $(TARGETNAME).sym

TAGS: $(TAG_OBJECTS)
	etags $(addprefix -i , $(TAG_OBJECTS))

$(TARGETNAME).map: $(TARGETNAME).nostrip.exe
ifeq ($(FULL_MAP),yes)
	$(OBJDUMP) -d -S $(TARGETNAME).nostrip.exe > $(TARGETNAME).map
else
	$(NM) --numeric-sort $(TARGETNAME).nostrip.exe > $(TARGETNAME).map
endif

$(TARGETNAME).o: $(OBJECTS)
	$(LD) \
		-r \
		-o $(TARGETNAME).o \
		$(OBJECTS)

$(PATH_TO_TOP)/include/reactos/bugcodes.h bugcodes.rc: ntoskrnl.mc
	$(MC) \
		-H $(PATH_TO_TOP)/include/reactos/bugcodes.h \
		-o bugcodes.rc \
		$(TARGETNAME).mc

$(DDK_PATH_LIB)/$(TARGETNAME).a: $(TARGETNAME).def
	- $(DLLTOOL) \
		--dllname $(TARGETNAME).exe \
		--def $(TARGETNAME).def \
		--output-lib $(DDK_PATH_LIB)/$(TARGETNAME).a \
		--kill-at

implib: $(DDK_PATH_LIB)/$(TARGETNAME).a

clean:
	- $(RM) $(OBJECTS_PATH)/*.o cc/*.o cm/*.o dbg/*.o dbg/i386/*.o ex/*.o \
              ex/i386/*.o inbv/*.o io/*.o ke/*.o ldr/*.o mm/*.o nt/*.o ob/*.o ps/*.o \
              rtl/*.o rtl/i386/*.o se/*.o ke/i386/*.o mm/i386/*.o fs/*.o po/*.o \
              lpc/*.o kd/*.o $(TARGETNAME).o junk.tmp base.tmp temp.exp \
              $(TARGETNAME).exe $(TARGETNAME).nostrip.exe $(TARGETNAME).sym ntoskrnl.map \
	          $(TARGETNAME).coff bugcodes.rc msg?????.bin $(DEP_FILES) \
	      $(TAG_OBJECTS) $(PATH_TO_TOP)/include/reactos/bugcodes.h


.PHONY: clean

ifneq ($(BOOTCD_INSTALL),)

install:
	- $(CP) $(TARGETNAME).exe $(INSTALL_DIR)/$(TARGETNAME).exe

else # BOOTCD_INSTALL

install:
	- $(CP) $(TARGETNAME).exe $(INSTALL_DIR)/system32/$(TARGETNAME).exe
	- $(CP) $(TARGETNAME).sym $(INSTALL_DIR)/symbols/$(TARGETNAME).sym

endif # BOOTCD_INSTALL


.PHONY: install

bootcd:
	- $(CP) $(TARGETNAME).exe $(BOOTCD_DIR)/reactos/$(TARGETNAME).exe

.PHONY: bootcd


ex/napi.o: ex/napi.c $(PATH_TO_TOP)/include/ntdll/napi.h

ke/main.o: ke/main.c $(PATH_TO_TOP)/include/reactos/buildno.h

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@
%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM_CMD) $(NFLAGS) $< -o $@
%.coff: %.rc
	$(RC) $(RCFLAGS) $< -o $@

#
#
#
.%.TAG: %.S
	etags -o $@ $<

.%.TAG: %.s
	etags -o $@ $<

.%.TAG: %.c
	etags -o $@ $^


docu:
	doxygen Doxyfile

.PHONY: docu


# EOF
