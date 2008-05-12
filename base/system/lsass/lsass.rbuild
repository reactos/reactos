<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="lsass" type="win32gui" installbase="system32" installname="lsass.exe" unicode="yes">
	<include base="lsass">.</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<library>advapi32</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>lsasrv</library>
	<file>lsass.c</file>
	<file>lsass.rc</file>
</module>
