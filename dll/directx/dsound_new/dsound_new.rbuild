<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dsound" type="win32dll" baseaddress="${BASEADDRESS_DSOUND}" installbase="system32" installname="dsound.dll" crt="msvcrt">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="dsound.spec" />
	<include base="dsound">.</include>
	<library>uuid</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>winmm</library>
	<library>dxguid</library>
	<library>setupapi</library>
	<library>ksuser</library>
	<file>capture.c</file>
	<file>capturebuffer.c</file>
	<file>classfactory.c</file>
	<file>devicelist.c</file>
	<file>directsound.c</file>
	<file>dsound.c</file>
	<file>enum.c</file>
	<file>misc.c</file>
	<file>notify.c</file>
	<file>primary.c</file>
	<file>property.c</file>
	<file>regsvr.c</file>
	<file>secondary.c</file>
	<file>stubs.c</file>
	<file>version.rc</file>
</module>
