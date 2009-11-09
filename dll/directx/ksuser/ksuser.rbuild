<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ksuser" type="win32dll" baseaddress="${BASEADDRESS_KSUSER}" installbase="system32" installname="ksuser.dll">
	<importlibrary definition="ksuser.spec" />
	<include base="ksuser">.</include>
	<library>advapi32</library>	
	<library>kernel32</library>
	<library>ntdll</library>
	<file>ksuser.c</file>
	<file>ksuser.rc</file>
</module>
