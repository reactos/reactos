<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="main" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_MAIN}" installbase="system32" installname="main.cpl" unicode="yes">
	<importlibrary definition="main.def" />
	<include base="main">.</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>devmgr</library>
	<library>comdlg32</library>
	<library>shell32</library>
	<library>gdi32</library>
	<library>msvcrt</library>
	<file>keyboard.c</file>
	<file>main.c</file>
	<file>mouse.c</file>
	<file>main.rc</file>
</module>
