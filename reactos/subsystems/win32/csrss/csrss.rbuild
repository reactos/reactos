<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="csrss" type="nativecui" installbase="system32" installname="csrss.exe">
		<include base="csrss">.</include>
		<include base="csrss">include</include>
		<include base="ReactOS">include/reactos/subsys</include>
		<include base="ReactOS">include/reactos/drivers</include>
		<define name="__USE_W32API" />
		<define name="_WIN32_WINNT">0x0600</define>
		<define name="WINVER">0x0501</define>
		<library>nt</library>
		<library>ntdll</library>
		<library>smdll</library>
		<directory name="api">
			<file>handle.c</file>
			<file>process.c</file>
			<file>user.c</file>
			<file>wapi.c</file>
		</directory>
		<pch>csrss.h</pch>
		<file>csrss.c</file>
		<file>init.c</file>
		<file>print.c</file>
		<file>video.c</file>
		<file>csrss.rc</file>
	</module>
	<directory name="win32csr">
		<xi:include href="win32csr/win32csr.rbuild" />
	</directory>
</group>
