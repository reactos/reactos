<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="samsrv" type="win32dll" baseaddress="${BASEADDRESS_SAMSRV}" entrypoint="0" installbase="system32" installname="samsrv.dll" unicode="yes">
	<importlibrary definition="samsrv.spec" />
	<include base="samsrv">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<file>samsrv.c</file>
	<file>samsrv.rc</file>
</module>
