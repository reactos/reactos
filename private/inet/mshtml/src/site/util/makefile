!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\util=)
MODULE=siteutil
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(SRCROOT)\site\base $(SRCROOT)\site\text $(SRCROOT)\site\style
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\miscelem $(SRCROOT)\site\ole

PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc


LIBTARG=$(BLDDIR)\siteutil.lib
LIBOBJS= \
    $(BLDDIR)\ie4intl.obj       \
    $(BLDDIR)\intl.obj          \
    $(BLDDIR)\collect.obj       \
    $(BLDDIR)\location.obj      \
    $(BLDDIR)\datcache.obj      \
    $(BLDDIR)\fcache.obj        \
    $(BLDDIR)\cfpf.obj          \
    $(BLDDIR)\array.obj         \
    $(BLDDIR)\mime64.obj        \
    $(BLDDIR)\strbuf.obj        \
    $(BLDDIR)\numconv.obj       \
    $(BLDDIR)\propbag.obj       \
    $(BLDDIR)\growarr.obj       \
    $(BLDDIR)\geturl.obj        \
    $(BLDDIR)\omrect.cxx        \
    $(BLDDIR)\collbase.cxx      \
    $(BLDDIR)\sbbase.obj        \
    $(BLDDIR)\sbutil.obj        \
    $(BLDDIR)\shape.obj

all:  $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
