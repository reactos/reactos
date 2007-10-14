<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="basesrv" type="win32dll">
	<importlibrary definition="basesrv.def" />
	<include base="basesrv">.</include>
	<include base="csr">include</include>
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>csrsrv</library>
	<file>main.c</file>
	<file>init.c</file>
	<file>server.c</file>
	<file>basesrv.rc</file>
</module>
