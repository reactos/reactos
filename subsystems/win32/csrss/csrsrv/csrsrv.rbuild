<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="csrsrv" type="nativedll" baseaddress="${BASEADDRESS_CSRSRV}" entrypoint="DllMain@12" installbase="system32" installname="csrsrv.dll">
	<importlibrary definition="csrsrv.spec" />
	<include base="csrsrv">.</include>
	<include base="csrss">.</include>
	<include base="csrss">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<library>ntdll</library>
	<library>pseh</library>
	<library>smdll</library>
	<library>smlib</library>
	<directory name="api">
		<file>process.c</file>
		<file>user.c</file>
		<file>wapi.c</file>
	</directory>
	<file>procsup.c</file>
	<file>thredsup.c</file>
	<file>init.c</file>
	<file>wait.c</file>
	<file>session.c</file>
	<file>server.c</file>
	<pch>srv.h</pch>
</module>
