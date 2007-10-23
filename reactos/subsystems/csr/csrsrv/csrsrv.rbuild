<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="csrsrv" type="nativedll">
	<importlibrary definition="csrsrv.def" />
	<include base="csrsrv">.</include>
	<include base="csr">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>ntdll</library>
	<library>pseh</library>
	<library>intrlck</library>
	<file>api.c</file>
	<file>init.c</file>
	<file>process.c</file>
	<file>server.c</file>
	<file>session.c</file>
	<file>thread.c</file>
	<file>wait.c</file>
	<pch>srv.h</pch>
</module>
