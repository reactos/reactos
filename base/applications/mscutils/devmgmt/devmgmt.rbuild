<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="devmgmt" type="win32gui" installbase="system32" installname="devmgmt.exe" unicode="yes">
<include base="devmgmt">.</include>
	<library>ntdll</library>
	<library>setupapi</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>advapi32</library>
	<library>devmgr</library>
	<compilationunit name="unit.c">
		<file>about.c</file>
		<file>devmgmt.c</file>
		<file>enumdevices.c</file>
		<file>mainwnd.c</file>
		<file>misc.c</file>
	</compilationunit>
	<file>devmgmt.rc</file>
	<pch>precomp.h</pch>
</module>
