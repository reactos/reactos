<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="loadperf" type="win32dll" baseaddress="${BASEADDRESS_LOADPERF}" installbase="system32" installname="loadperf.dll" allowwarnings="true">
	<importlibrary definition="loadperf.spec" />
	<include base="loadperf">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>loadperf_main.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>

