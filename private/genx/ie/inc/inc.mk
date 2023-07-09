###############################################################################
#
#  Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1996-1998
#       All Rights Reserved.
#
#       Internet SDK include files
#
###############################################################################
!ifndef ARCH
ARCH     =i386
ARCHTOOLS=x86
!endif

IS_32    =TRUE
IS_SDK   =TRUE
WANT_C1032=TRUE
DEVTOOLS =$(ROOT)\dev\tools\binw\$(ARCHTOOLS)
SDKINCDIR=$(ROOT)\dev\sdk\inc
INCDIR=$(ROOT)\dev\inc
COMMONTOOLS=$(ROOT)\dev\tools\common

PROXY=..\proxy
MIDL      =$(DEVTOOLS)\midl.exe
MIDLFLAGS = /client none /server none /ms_ext /c_ext /env win32 /Oic  -D_MIDL_USER_MARSHAL_DISABLED=1
TLBFLAGS  = -o Errors.log \
		-cpp_opt "-I.. -I. /C /E /D__MKTYPLIB__ -nologo "


# List of main dependents
LOCLIST= comcat.h docobj.h hlink.h hliface.h urlmon.h urlhist.h \
	 inetsdk.h wininet.h urlcache.h servprov.h htiframe.h htiface.h exdisp.h activaut.h activscp.h \
	 activdbg.h procdm.h objsafe.h mimeinfo.h

DEPLIST= $(SDKINCDIR)\comcat.h $(SDKINCDIR)\docobj.h \
	 $(SDKINCDIR)\hlink.h $(SDKINCDIR)\hliface.h $(SDKINCDIR)\urlmon.h \
	 $(SDKINCDIR)\inetsdk.h \
	 $(SDKINCDIR)\wininet.h \
        $(INCDIR)\wininet.h \
	 $(SDKINCDIR)\urlcache.h $(SDKINCDIR)\htiface.h $(SDKINCDIR)\htiframe.h \
     $(SDKINCDIR)\servprov.h \
	 $(SDKINCDIR)\exdispid.h       \
	 $(SDKINCDIR)\hlinkez.h        \
         $(SDKINCDIR)\activaut.h \
	 $(SDKINCDIR)\activscp.h \
	 $(SDKINCDIR)\activdbg.h \
	 $(SDKINCDIR)\procdm.h \
	 $(SDKINCDIR)\objsafe.h \
     $(SDKINCDIR)\urlhist.h \
     $(SDKINCDIR)\mimeinfo.h \
	 ..\retail\$(ARCH)\exdisp.tlb

CLEANLIST=$(DEPLIST) $(LOCLIST) *.x Errors.log

MAKE: $(DEPLIST)

###### Don't move this line #######
!include $(ROOT)\dev\master.mk
###################################

..\retail\$(ARCH)\exdisp.tlb: exdisp.odl
	mktyplib $(TLBFLAGS) -tlb ..\retail\$(ARCH)\exdisp.tlb -h exdisp.h $?
	copy exdisp.h $(SDKINCDIR)

$(SDKINCDIR)\exdispid.h: exdispid.h
	copy exdispid.h $(SDKINCDIR)

$(SDKINCDIR)\shdispid.h: shdispid.h
	copy shdispid.h $(SDKINCDIR)

$(SDKINCDIR)\comcat.h comcat.h: comcat.idl

$(SDKINCDIR)\activaut.h activaut.h: activaut.idl
	$(MIDL) $(MIDLFLAGS) /header $(*B).h \
	/iid ..\uuid\$(*B).c $?
	copy $(*B).h $(SDKINCDIR)

$(SDKINCDIR)\activscp.h activscp.h: activscp.idl
	$(MIDL) $(MIDLFLAGS) /header $(*B).h \
	/iid ..\uuid\$(*B).c $?
	copy $(*B).h $(SDKINCDIR)

$(SDKINCDIR)\activdbg.h activdbg.h: activdbg.idl
	$(MIDL) $(MIDLFLAGS) /header $(*B).h \
	/iid ..\uuid\$(*B).c $?
	copy $(*B).h $(SDKINCDIR)

$(SDKINCDIR)\procdm.h procdm.h: procdm.idl
	$(MIDL) $(MIDLFLAGS) /header $(*B).h \
	/iid ..\uuid\$(*B).c $?
	copy $(*B).h $(SDKINCDIR)

$(SDKINCDIR)\objsafe.h objsafe.h: objsafe.idl
	$(MIDL) $(MIDLFLAGS) /header $(*B).h \
	/iid ..\uuid\$(*B).c $?
	copy $(*B).h $(SDKINCDIR)

$(SDKINCDIR)\urlmon.h urlmon.h: urlmon.idl
	$(MIDL) $(MIDLFLAGS) /header $(*B).h \
        /iid ..\uuid\$(*B).c $?
	copy $(*B).h $(SDKINCDIR)

$(SDKINCDIR)\urlhist.h urlhist.h: urlhist.idl
	$(MIDL) $(MIDLFLAGS) /header $(*B).h \
        /iid ..\uuid\$(*B).c $?
	copy $(*B).h $(SDKINCDIR)

$(SDKINCDIR)\docobj.h docobj.h: docobj.idl

$(SDKINCDIR)\hliface.h hliface.h: hliface.idl

$(SDKINCDIR)\htiframe.h htiframe.h: htiframe.idl

$(SDKINCDIR)\htiface.h htiface.h: htiface.idl

$(SDKINCDIR)\mimeinfo.h mimeinfo.h: mimeinfo.idl

$(SDKINCDIR)\hlink.h hlink.h: hlink.idl

$(SDKINCDIR)\hlinkez.h: hlinkez.h
	copy hlinkez.h $(SDKINCDIR)

$(SDKINCDIR)\inetsdk.h inetsdk.h: inetsdk.idl

$(SDKINCDIR)\servprov.h servprov.h: servprov.idl

$(SDKINCDIR)\urlmon.h urlmon.h: urlmon.idl

$(SDKINCDIR)\urlcache.h urlcache.h: urlcache.w

wininet.h: wininet.w

$(SDKINCDIR)\wininet.h $(INCDIR)\wininet.h : wininet.h

inetsdk.idl: comcat.idl docobj.idl urlmon.idl hlink.idl activaut.idl activscp.idl activdbg.idl procdm.idl objsafe.idl

.idl.h:
	$(MIDL) $(MIDLFLAGS) /dlldata ..\proxy\dlldata.c /header $(*B).h \
	/iid ..\uuid\$(*B).c /proxy ..\proxy\$(*B).c $?
	copy $(*B).h $(SDKINCDIR)

.idl{$(SDKINCDIR)}.h:
	$(MIDL) $(MIDLFLAGS) /dlldata ..\proxy\dlldata.c /header $(*B).h \
	/iid ..\uuid\$(*B).c /proxy ..\proxy\$(*B).c $?
	copy $(*B).h $(SDKINCDIR)


.w.h:
	-del  $(*B).x > NUL
	-del  $(*B).p > NUL
	$(DEVTOOLS)\hsplit -4 -o $(*B).x $(*B).p $(*B).w
	$(DEVTOOLS)\wcshdr < $(*B).x > $(*B).h
	del  $(*B).x
	-del  $(*B).p > NUL

.h{$(INCDIR)}.h:
	copy $(*B).h $(INCDIR)

.h{$(SDKINCDIR)}.h:
	$(COMMONTOOLS)\mkpublic $(*B).h $(SDKINCDIR)\$(*B).h

.odl{..\retail\$(ARCH)}.tlb:
.odl.tlb:
	mktyplib $(TLBFLAGS) -tlb ..\retail\$(ARCH)\$*.tlb $*.odl


# Add suffixes for MIDL compiler, Type Lib, and UNICODE coversion
.SUFFIXES: .idl .w .odl .tlb
