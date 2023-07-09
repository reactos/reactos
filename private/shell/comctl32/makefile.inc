..\commctrl.rc : selfreg_comctl32.inf

!include $(CCSHELL_DIR)\makefile.inc

LOC_DIR = $(CCSHELL_DIR)\comctl32\loc
RMERGE  = $(LOC_DIR)\$(TARGET_DIRECTORY)\rmerge.exe

localizecc32: $(_NTTREE)\comctl32.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\ar.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\br.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\ca.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\cn.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\cs.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\da.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\de.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\el.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\es.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\fi.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\fr.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\hu.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\it.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\iw.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\ja.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\ja2.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\ko.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\nl.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\no.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\pl.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\pt.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\ru.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\sl.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\sv.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\tr.dll
        $(RMERGE) $(_NTTREE)\comctl32.dll $(LOC_DIR)\tw.dll


