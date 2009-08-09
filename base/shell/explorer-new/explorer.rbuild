<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="explorer_new" type="win32gui" installname="explorer_new.exe" unicode="true">
	<include base="explorer_new">.</include>
	<define name="WIN32" />
	<library>advapi32</library>
	<library>kernel32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<library>uuid</library>
	<pch>precomp.h</pch>
	<file>desktop.c</file>
	<file>dragdrop.c</file>
	<file>explorer.c</file>
	<file>startmnu.c</file>
	<file>taskband.c</file>
	<file>taskswnd.c</file>
	<file>tbsite.c</file>
	<file>trayntfy.c</file>
	<file>trayprop.c</file>
	<file>traywnd.c</file>
	<file>explorer.rc</file>
</module>
