<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dsound_new" type="win32dll" baseaddress="${BASEADDRESS_DSOUND}" installbase="system32" installname="dsound_new.dll" crt="msvcrt">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="dsound.spec" />
	<include base="dsound">.</include>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>winmm</library>
	<library>dxguid</library>
	<file>classfactory.c</file>
	<file>dsound.c</file>
	<file>stubs.c</file>
	<file>version.rc</file>
</module>
