<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mmcclient" type="win32gui" installbase="system32" installname="mmc.exe" unicode="yes" allowwarnings="true">
	<include base="mmcclient">.</include>
	<library>user32</library>
	<library>gdi32</library>
	<library>comdlg32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>comctl32</library>
	<library>ole32</library>
	<library>mmclib</library>
     <library>kernel32</library>
     <library>ws2_32</library>
	<library>uuid</library>
     <file>main.cpp</file>
     <file>register_functions.cpp</file>
     <file>snapin.cpp</file>
     <file>fonction.cpp</file>
     <file>MainWindow.cpp</file>
     <file>ressource.rc</file>
</module>
