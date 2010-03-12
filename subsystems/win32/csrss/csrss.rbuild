<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="csrss" type="nativecui" installbase="system32" installname="csrss.exe">
		<include base="csrss">.</include>
		<include base="csrss">include</include>
		<include base="ReactOS">include/reactos/subsys</include>
		<include base="ReactOS">include/reactos/drivers</include>
		<compilerflag compilerset="gcc">-fms-extensions</compilerflag>
		<library>nt</library>
		<library>ntdll</library>
		<library>csrsrv</library>
		<file>csrss.c</file>
		<file>csrss.rc</file>
	</module>
	<directory name="win32csr">
		<xi:include href="win32csr/win32csr.rbuild" />
	</directory>
	<directory name="csrsrv">
		<xi:include href="csrsrv/csrsrv.rbuild" />
	</directory>
</group>
