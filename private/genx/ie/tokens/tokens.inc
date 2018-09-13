!ifndef BUILDTYPE
BUILDTYPE=release
!endif

TOK_ADDRESS	   = http://iedist/build
TOK_DESKTOP	   = http://iedist/build/desktop/bld$(TOK_BUILDNUM)
ADDRESS            = $(TOK_ADDRESS)/bld$(TOK_BUILDNUM)

TOK_FIRSTNT_PAGE   = $(ADDRESS)/ie4setup/default.htm
TOK_FIRSTW95_PAGE  = $(ADDRESS)/ie4setup/default.htm
TOK_DESKCHANNEL1   = $(TOK_DESKTOP)/cguide/loaddflt.htm
TOK_DESKCHANNEL2   = $(TOK_DESKTOP)/cguide/chnhtm.htm
