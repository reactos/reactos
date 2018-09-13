# special stuff for evtmon
# the -D's come from the office guys' original 'emtest' sample app
# WARNING: DEBUG implies DYN_RULE for some reason
EM_DEFINES  =
EM_DEFINES  = $(EM_STUFF) \
    -DSTANDALONE -DSTANDALONE_WIN32 \
    -DSTATIC_INIT -DANSI_XCHAR

# static  EM-supplied    hdrs/srcs come from evtmon/inc
# dynamic rulc-generated hdrs/srcs come from -I$O
# the -I$O is semi-weird but they're config-dependent so they need to go there
EM_INCLUDES =
EM_INCLUDES = $(EM_INCLUDES);$(CCSHELL_DIR)\inc\evtmon
EM_INCLUDES = $(EM_INCLUDES);$O

# rulc generated headers, from ierules.rul
# we'd like to key off of all of these, but they're not always
# created (sigh), so for now we key off emeval.c, which always is
YYHDR   = \
        $O\emdef.h $O\emacts.h $O\emactr.h $O\emruli.h \
        $O\emeval.c $O\emsqck.c_ $O\emact.c_
YYHDR   = $O\emeval.c

YYSRC   = $(CCSHELL_DIR)\lib\genem.c
# BUGBUG tmp hack until we put genem.c in shell/lib/evtmon
YYSRC   = $(CCSHELL_DIR)\inc\evtmon\genem.c

$O\libem.obj : libem.c $(YYSRC) $(YYHDR)

# rulc puts its stuff in "." so we need to pushd/popd around it
# also rulc doesn't (re)gen 'unneeded' files so we need to cleanup beforehand
# we 
$(YYHDR) : ierules.rul
        $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS) ierules.rul > $(O)\ierules.rl
        cd $O
        del emdef.h emactr.h emacts.h emruli.h
        del emeval.c emact.c_ emfilt.c
        rulc ierules.rl
        cd $(CCSHELL_DIR)\browseui\emclient


!include $(CCSHELL_DIR)\makefile.inc
