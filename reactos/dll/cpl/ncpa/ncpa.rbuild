<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ncpa" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_NCPA}" installbase="system32" installname="ncpa.cpl" unicode="yes">
	<importlibrary definition="ncpa.spec" />
	<include base="ncpa">.</include>

	<define name="_WIN32_WINNT">0x600</define>

	<library>kernel32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<file>ncpa.c</file>
	<file>ncpa.spec</file>
</module>
