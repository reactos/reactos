preall: all

PATH_TO_TOP := ..

include $(PATH_TO_TOP)/config

TARGET_REGTESTS = yes

TARGET_TYPE = kernel

TARGET_NAME = ntoskrnl

TARGET_BOOTSTRAP = yes

CONFIG :=

ifeq ($(DBG), 1)
DBG_OR_KDBG := 1
endif
ifeq ($(KDBG), 1)
DBG_OR_KDBG := 1
endif

LINKER_SCRIPT := ntoskrnl.lnk
STRIP_FLAGS := -Wl,-s

TARGET_ASFLAGS = -I./include
TARGET_CFLAGS = -I./include $(CFLAGS_KDBG) -Wall -Werror $(CFLAGS_OPT)

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

# 3GB User Mode Memory Space support
ifeq ($(3GB), 1)
TARGET_CFLAGS += -D__3GB__
TARGET_BASE = 0xC0000000
else
TARGET_BASE = 0x80000000
endif

ifneq ($(DBG), 0)
TARGET_CFLAGS += -DDBG
endif

# enable thread event pair features (NT4 only!)
# TARGET_CFLAGS += -D_ENABLE_THRDEVTPAIR

#
# Javascript extension for kdb
#

OBJECTS_PATH = objects

#
# Architecture specific Makefile
# Defines $(OBJECTS_ARCH)
#
include Makefile.$(ARCH)

# Run-Time Library (Rtl)
OBJECTS_RTL = \
	rtl/atom.o \
	rtl/capture.o \
	rtl/ctype.o \
	rtl/debug.o \
	rtl/handle.o \
	rtl/message.o \
	rtl/misc.o \
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
	ke/clock.o \
	ke/dpc.o \
	ke/device.o \
	ke/event.o \
	ke/gate.o \
	ke/gmutex.o \
	ke/kqueue.o \
	ke/kthread.o \
	ke/ipi.o \
	ke/main.o \
	ke/mutex.o \
	ke/process.o \
	ke/profile.o \
	ke/queue.o \
	ke/sem.o \
	ke/spinlock.o \
	ke/timer.o \
	ke/usercall.o \
	ke/wait.o

# Memory Manager (Mm)
OBJECTS_MM = \
	mm/anonmem.o \
	mm/aspace.o \
	mm/balance.o \
	mm/cont.o \
	mm/drvlck.o \
	mm/elf32.o \
	mm/elf64.o \
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
	mm/pe.o \
	mm/pool.o \
	mm/ppool.o \
	mm/physical.o \
	mm/process.o \
	mm/region.o \
	mm/rmap.o \
	mm/section.o \
	mm/verifier.o \
	mm/virtual.o \
	mm/wset.o

# I/O Subsystem (Io)
OBJECTS_IO = \
	io/adapter.o \
	io/arcname.o \
	io/bootlog.o \
	io/controller.o \
	io/device.o \
	io/deviface.o \
	io/disk.o \
	io/driver.o \
	io/efi.o \
	io/error.o \
	io/event.o \
	io/file.o \
	io/fs.o \
	io/iocomp.o \
	io/iomgr.o \
	io/iowork.o \
	io/irp.o \
	io/irq.o \
	io/mdl.o \
	io/plugplay.o \
	io/pnpdma.o \
	io/pnpmgr.o \
	io/pnpnotify.o \
	io/pnpreport.o \
	io/pnproot.o \
	io/rawfs.o \
	io/remlock.o \
	io/resource.o \
	io/share.o \
	io/symlink.o \
	io/timer.o \
	io/vpb.o \
	io/wmi.o \

# Object Manager (Ob)
OBJECTS_OB = \
	ob/dirobj.o \
	ob/handle.o \
	ob/namespc.o \
	ob/ntobj.o \
	ob/object.o \
	ob/sdcache.o \
	ob/security.o \
	ob/symlink.o \
	ob/wait.o

# Process Manager (Ps)
OBJECTS_PS = \
	ps/cid.o \
	ps/debug.o \
	ps/idle.o \
	ps/job.o \
	ps/kill.o \
	ps/locale.o \
	ps/process.o \
	ps/psmgr.o \
	ps/notify.o \
	ps/quota.o \
	ps/query.o \
	ps/security.o \
	ps/suspend.o \
	ps/thread.o \
	ps/win32.o

# Executive Subsystem (Ex)
OBJECTS_EX = \
	ex/callback.o \
	ex/dbgctrl.o \
	ex/error.o \
	ex/event.o \
	ex/evtpair.o \
	ex/fmutex.o \
	ex/handle.o \
	ex/init.o \
	ex/interlck.o \
	ex/list.o \
	ex/lookas.o \
	ex/mutant.o \
	ex/power.o \
	ex/profile.o \
	ex/resource.o \
	ex/rundown.o \
	ex/sem.o \
	ex/synch.o \
	ex/sysinfo.o \
	ex/time.o \
	ex/timer.o \
	ex/util.o \
	ex/uuid.o \
	ex/win32k.o \
	ex/work.o \
	ex/zone.o \
	ex/zw.o

# Installable File System Run-Time Library (FsRtl)
OBJECTS_FS = \
	fs/context.o \
	fs/fastio.o \
	fs/filelock.o \
	fs/mcb.o \
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
	cc/view.o

# Kernel Debugger Support (Kd)
OBJECTS_KD = \
	kd/kdinit.o \
	kd/kdmain.o \
	kd/kdio.o \
    
ifeq ($(DBG), 1)
OBJECTS_KD := \
	$(OBJECTS_KD) \
	kd/wrappers/gdbstub.o \
	kd/wrappers/bochs.o
endif

# User-Mode Debugging (Dbgk)
OBJECTS_DBGK = \
	dbgk/dbgkutil.o \
	dbgk/debug.o

ifeq ($(KDBG), 1)
OBJECTS_KDBG := \
	kdbg/kdb.o \
	kdbg/kdb_cli.o \
	kdbg/kdb_expr.o \
	kdbg/kdb_keyboard.o \
	kdbg/kdb_serial.o \
	kdbg/kdb_string.o
preall: all
else
OBJECTS_KDBG :=
endif
ifeq ($(DBG_OR_KDBG), 1)
OBJECTS_KDBG := \
	$(OBJECTS_KDBG) \
	kdbg/kdb_symbols.o
endif

# Boot video (Inbv)
OBJECTS_INBV = \
	inbv/inbv.o

DEP_OBJECTS := $(OBJECTS_MM) $(OBJECTS_ARCH) \
	$(OBJECTS_IO) $(OBJECTS_KE) $(OBJECTS_OB) \
	$(OBJECTS_PS) $(OBJECTS_EX) $(OBJECTS_CC) $(OBJECTS_FS) $(OBJECTS_SE) \
	$(OBJECTS_KDBG) $(OBJECTS_DBGK) $(OBJECTS_CM) $(OBJECTS_LDR) $(OBJECTS_LPC) \
	$(OBJECTS_PO) $(OBJECTS_KD) $(OBJECTS_RTL) $(OBJECTS_INBV)

TAG_OBJECTS := $(join $(dir $(DEP_OBJECTS)),$(patsubst %.o, .%.TAG, $(notdir $(DEP_OBJECTS))))

TARGET_PCH = include/ntoskrnl.h


# Resources
OBJECTS_RESOURCE = \
	$(TARGET_NAME).coff

$(OBJECTS_PATH):
	mkdir $(OBJECTS_PATH)

$(OBJECTS_RESOURCE): $(TARGET_NAME).rc bugcodes.rc $(PATH_TO_TOP)/include/reactos/resource.h

# Note: arch.o MUST be the first file!!!
OBJECTS := \
	$(OBJECTS_ARCH) \
	$(OBJECTS_KE) \
	$(OBJECTS_CC) \
	$(OBJECTS_CM) \
	$(OBJECTS_EX) \
	$(OBJECTS_LPC) \
	$(OBJECTS_FS) \
	$(OBJECTS_IO) \
	$(OBJECTS_KD) \
	$(OBJECTS_DBGK) \
	$(OBJECTS_LDR) \
	$(OBJECTS_MM) \
	$(OBJECTS_OB) \
	$(OBJECTS_PO) \
	$(OBJECTS_PS) \
	$(OBJECTS_RTL) \
	$(OBJECTS_SE) \
	$(OBJECTS_KDBG) \
	$(OBJECTS_INBV) \
	$(OBJECTS_RESOURCE)

EXTRA_OBJECTS = $(PATH_TO_TOP)/include/reactos/bugcodes.h

TARGET_OBJECTS = $(EXTRA_OBJECTS) $(OBJECTS)

TARGET_LIBS = \
	$(DDK_PATH_LIB)/libhal.a \
	$(SDK_PATH_LIB)/librtl.a \
	$(SDK_PATH_LIB)/librossym.a \
	$(SDK_PATH_LIB)/libstring.a \
	$(SDK_PATH_LIB)/librosrtl.a \
	$(SDK_PATH_LIB)/libpseh.a \
	$(SDK_PATH_LIB)/libwdmguid.a \
	$(PATH_TO_TOP)/drivers/lib/csq/csq.o

TARGET_LFLAGS = \
	-Wl,-T,ntoskrnl.lnk \
	-Wl,--subsystem,native \
	-Wl,--image-base,$(TARGET_BASE) \
	-Wl,--file-alignment,0x1000 \
	-Wl,--section-alignment,0x1000 \
	-Wl,--entry,_NtProcessStartup \
	-nostdlib

TARGET_GCCLIBS = gcc

TAGS: $(TAG_OBJECTS)
	etags $(addprefix -i , $(TAG_OBJECTS))

GENERATED_HEADER_FILES := $(PATH_TO_TOP)/include/reactos/bugcodes.h

$(PATH_TO_TOP)/include/reactos/bugcodes.h bugcodes.rc: ntoskrnl.mc
	$(MC) \
		-H $(PATH_TO_TOP)/include/reactos/bugcodes.h \
		-o bugcodes.rc \
		$(TARGET_NAME).mc

TARGET_CLEAN = \
  $(PATH_TO_TOP)/include/reactos/bugcodes.h \
  $(DEP_OBJECTS) $(DEP_FILES) MSG00409.bin bugcodes.rc


ke/main.o: ke/main.c $(PATH_TO_TOP)/include/reactos/buildno.h

$(TARGET_PCH): $(PATH_TO_TOP)/include/reactos/bugcodes.h


.%.TAG: %.S
	etags -o $@ $<

.%.TAG: %.s
	etags -o $@ $<

.%.TAG: %.c
	etags -o $@ $^


docu:
	doxygen Doxyfile

.PHONY: docu

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
