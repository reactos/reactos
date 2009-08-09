<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dsound" type="win32dll" baseaddress="${BASEADDRESS_DSOUND}" installbase="system32" installname="dsound.dll" crt="msvcrt">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="dsound.spec" />
	<include base="dsound">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>winmm</library>
	<library>dxguid</library>
	<file>version.rc</file>
	<file>buffer.c</file>
	<file>capture.c</file>
	<file>dsound.c</file>
	<file>dsound_convert.c</file>
	<file>dsound_main.c</file>
	<file>duplex.c</file>
	<file>mixer.c</file>
	<file>primary.c</file>
	<file>propset.c</file>
	<file>regsvr.c</file>
	<file>sound3d.c</file>
</module>
