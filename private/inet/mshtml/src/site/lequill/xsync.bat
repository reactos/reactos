@echo off
rem XSync batch file for Trident
rem Uses a single enlistment.

cd /d d:\nt\private\inet\mshtml\src\site\lequill
ssync
out qviewserv.cxx qbase.cxx qtreeserv.cxx

rem base directory
cd /d d:\nt\private\inet\mshtml\src\site\base
out treeserv.cxx viewserv.cxx
copy ..\lequill\qtreeserv.cxx treeserv.cxx
copy ..\lequill\qviewserv.cxx viewserv.cxx
ssync treeserv.cxx viewserv.cxx
copy treeserv.cxx ..\lequill\qtreeserv.cxx
copy viewserv.cxx ..\lequill\qviewserv.cxx
in -i treeserv.cxx viewserv.cxx
ssync treeserv.cxx viewserv.cxx
out treeserv.cxx viewserv.cxx

rem cdbase directory
cd /d d:\nt\private\inet\mshtml\src\core\cdbase
out base.cxx
copy d:\nt\private\inet\mshtml\src\site\lequill\qbase.cxx base.cxx
ssync base.cxx
copy base.cxx d:\nt\private\inet\mshtml\src\site\lequill\qbase.cxx
in -i base.cxx
ssync base.cxx
out base.cxx
