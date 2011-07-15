<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="csrsrv" type="nativedll">
	<importlibrary definition="csrsrv.spec" />
	<include base="csrsrv">.</include>
	<include base="csr">.</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<library>ntdll</library>
	<library>pseh</library>
	<file>api.c</file>
	<file>init.c</file>
	<file>process.c</file>
	<file>server.c</file>
	<file>session.c</file>
	<file>thread.c</file>
	<file>wait.c</file>
	<pch>srv.h</pch>
</module>
