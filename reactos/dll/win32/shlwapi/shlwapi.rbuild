<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="shlwapi" type="win32dll" baseaddress="${BASEADDRESS_SHLWAPI}" installbase="system32" installname="shlwapi.dll">
	<importlibrary definition="shlwapi.spec" />
	<include base="shlwapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>assoc.c</file>
	<file>clist.c</file>
	<file>istream.c</file>
	<file>msgbox.c</file>
	<file>ordinal.c</file>
	<file>path.c</file>
	<file>reg.c</file>
	<file>regstream.c</file>
	<file>shlwapi_main.c</file>
	<file>stopwatch.c</file>
	<file>string.c</file>
	<file>thread.c</file>
	<file>url.c</file>
	<file>wsprintf.c</file>
	<file>shlwapi.rc</file>
	<library>wine</library>
	<library>uuid</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>oleaut32</library>
	<library>ole32</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<library>mpr</library>
	<library>mlang</library>
	<library>urlmon</library>
	<library>shell32</library>
	<library>winmm</library>
	<library>version</library>
	<library>ntdll</library>
</module>
</group>
