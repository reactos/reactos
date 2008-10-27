<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="msiexec" type="win32gui" installbase="system32" installname="msiexec.exe" allowwarnings="true">
	<include base="msiexec">.</include>
	<include base="ReactOS">include/wine</include>
	<library>uuid</library>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>msi</library>
	<file>msiexec.c</file>
	<file>rsrc.rc</file>
	<file>service.c</file>
	<file>version.rc</file>
</module>
