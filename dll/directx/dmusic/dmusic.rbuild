<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dmusic" type="win32dll" entrypoint="0" installbase="system32" installname="dmusic.dll" unicode="yes">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="dmusic.spec" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<define name="__WINESRC__" />
	<include base="dmusic">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>dxguid</library>
	<library>dsound</library>
	<file>version.rc</file>
	<file>buffer.c</file>
	<file>clock.c</file>
	<file>collection.c</file>
	<file>dmusic.c</file>
	<file>dmusic_main.c</file>
	<file>download.c</file>
	<file>downloadedinstrument.c</file>
	<file>instrument.c</file>
	<file>port.c</file>
	<file>regsvr.c</file>
</module>
