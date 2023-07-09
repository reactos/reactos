@echo off
cd ..\bin
out syncui.dll syncui.map syncui.sym
copy ..\syncui\debug\syncui.dll
copy ..\syncui\debug\syncui.map
copy ..\syncui\debug\syncui.sym
in -c "Latest build." syncui.dll syncui.map syncui.sym
cd ..\syncui

