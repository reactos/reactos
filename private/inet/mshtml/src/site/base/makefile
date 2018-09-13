!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\base=)
MODULE=sitebase
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(SRCROOT)\dlay\dl $(SRCROOT)\site\util
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\ctrl\include $(ROOT)\src\ctrl\types
INCLUDEDIRS=$(INCLUDEDIRS) $(ROOT) $(SRCROOT)\core\cdutil $(SRCROOT)\site\text $(SRCROOT)\site\style $(SRCROOT)\site\dbind
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\other\moniker $(SRCROOT)\site\miscelem $(SRCROOT)\site\miscsite $(SRCROOT)\site\builtin
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\ole $(SRCROOT)\site\layout

PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\base.lib
LIBOBJS= \
	 $(BLDDIR)\attr.obj     \
	 $(BLDDIR)\bindhost.obj \
	 $(BLDDIR)\clstab.obj   \
	 $(BLDDIR)\csite.obj    \
	 $(BLDDIR)\ctxmenu.obj  \
	 $(BLDDIR)\cutil.obj    \
	 $(BLDDIR)\dataview.obj \
	 $(BLDDIR)\docdata.obj  \
	 $(BLDDIR)\element.obj  \
	 $(BLDDIR)\fframe.obj   \
	 $(BLDDIR)\folectrl.obj \
	 $(BLDDIR)\fpersist.obj \
	 $(BLDDIR)\formkrnl.obj \
	 $(BLDDIR)\formmso.obj  \
	 $(BLDDIR)\fprint.obj   \
	 $(BLDDIR)\fpropdlg.obj \
	 $(BLDDIR)\fscroll.obj  \
	 $(BLDDIR)\getid.obj    \
	 $(BLDDIR)\grab.obj     \
	 $(BLDDIR)\hedelems.obj \
	 $(BLDDIR)\hlink.obj    \
	 $(BLDDIR)\idobj.obj    \
	 $(BLDDIR)\inplace.obj  \
	 $(BLDDIR)\ipwnd.obj    \
	 $(BLDDIR)\krnldisp.obj \
	 $(BLDDIR)\doclyt.obj   \
	 $(BLDDIR)\meta.obj     \
	 $(BLDDIR)\omdoc.obj    \
	 $(BLDDIR)\optnobj.obj  \
	 $(BLDDIR)\paint.obj    \
	 $(BLDDIR)\progress.obj \
         $(BLDDIR)\recalchost.obj\
	 $(BLDDIR)\rootsite.obj \
	 $(BLDDIR)\rtftohtm.obj \
	 $(BLDDIR)\saver.obj    \
	 $(BLDDIR)\scollect.obj \
	 $(BLDDIR)\selctr.obj   \
	 $(BLDDIR)\selecobj.obj \
	 $(BLDDIR)\sholder.obj  \
	 $(BLDDIR)\siterang.obj \
	 $(BLDDIR)\stdform.obj  \
	 $(BLDDIR)\sui.obj      \
	 $(BLDDIR)\taborder.obj \
	 $(BLDDIR)\track.obj    \
	 $(BLDDIR)\unitmeas.obj \
	 $(BLDDIR)\unknown.obj  \
	 $(BLDDIR)\wfromd.obj   \
	 $(BLDDIR)\window.obj   \
	 $(BLDDIR)\xbag.obj     \
	 $(BLDDIR)\xobj.obj     \
	 $(BLDDIR)\xyl.obj      \
	 $(BLDDIR)\filter.obj   \

all: $(TYPETARG) $(LIBTARG)

!include $(MAKE2_INC)

!endif          # NTMAKEENV
