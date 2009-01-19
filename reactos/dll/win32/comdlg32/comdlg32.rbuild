<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="comdlg32" type="win32dll" baseaddress="${BASEADDRESS_COMDLG32}" installbase="system32" installname="comdlg32.dll">
	<importlibrary definition="comdlg32.spec" />
	<include base="comdlg32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>cdlg32.c</file>
	<file>colordlg.c</file>
	<file>filedlg.c</file>
	<file>filedlg31.c</file>
	<file>filedlgbrowser.c</file>
	<file>finddlg32.c</file>
	<file>fontdlg.c</file>
	<file>printdlg.c</file>
	<file>rsrc.rc</file>
	<library>wine</library>
	<library>uuid</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<library>comctl32</library>
	<library>winspool</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ole32</library>
	<library>ntdll</library>
</module>
</group>
