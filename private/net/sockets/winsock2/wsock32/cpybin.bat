if exist \i386\symbols\dll\wsock32.old del \i386\symbols\dll\wsock32.old
if exist \i386\symbols\dll\wsock32.dbg ren \i386\symbols\dll\wsock32.dbg wsock32.old
copy \nt\public\sdk\lib\i386\wsock32.dll \i386\symbols\dll

