<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="ntdll_apitest" type="win32cui" installbase="bin" installname="ntdll_apitest.exe"
    allowwarnings="true">
	<include base="ntdll_apitest">.</include>
	<library>wine</library>
	<library>advapi32</library>
	<library>ntdll</library>
	<library>pseh</library>
	<file>testlist.c</file>

	<file>NtAllocateVirtualMemory.c</file>
	<file>NtFreeVirtualMemory.c</file>
	<file>RtlInitializeBitMap.c</file>
	<file>ZwContinue.c</file>
	<file>SystemInfo.c</file>
	<if property="ARCH" value="i386">
		<directory name="i386">
			<file>ZwContinue.S</file>
		</directory>
	</if>
</module>
</group>
