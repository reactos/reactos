<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="srclient" type="win32dll" baseaddress="${BASEADDRESS_SRCLIENT}" installbase="system32" entrypoint="0" installname="srclient.dll" allowwarnings="true">
	<importlibrary definition="srclient.spec" />
	<include base="srclient">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<file>srclient_main.c</file>
</module>
