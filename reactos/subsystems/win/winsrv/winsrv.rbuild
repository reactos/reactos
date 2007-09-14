<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="winsrv" type="win32dll" entrypoint="0">
	<importlibrary definition="winsrv.def" />
	<include base="winsrv">.</include>
	<include base="csr">include</include>
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>csrsrv</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>dllmain.c</file>
	<file>init.c</file>
	<file>server.c</file>
	<file>winsrv.rc</file>
</module>
