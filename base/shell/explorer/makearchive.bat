set MAKE=mingw32-make.exe
set TARGET=E:\html\newhome\explorer

rm explorer-ansi.exe
%MAKE% -f Makefile-precomp clean all UNICODE=0
mv explorer.exe explorer-ansi.exe

%MAKE% -f Makefile-precomp clean all UNICODE=1
zip %TARGET%\ros-explorer.zip explorer-ansi.exe explorer.exe *.dll

cd ..\lean-explorer
%MAKE% -f Makefile-precomp clean all UNICODE=1
zip %TARGET%\lean-explorer.zip explorer.exe
cd ..\explorer

pack
mv explorer-src.zip %TARGET%\explorer-src.zip

make-full-docu
move ros-explorer.chm %TARGET%\
move ros-explorer-full.chm %TARGET%\
