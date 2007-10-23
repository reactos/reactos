<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="ntvdm" type="win32cui" installbase="system32" installname="ntvdm.exe">
	<include base="ntvdm">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<file>ntvdm.c</file>
	<file>ntvdm.rc</file>
</module>
