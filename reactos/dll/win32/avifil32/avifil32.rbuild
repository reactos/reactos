<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="avifil32" type="win32dll" baseaddress="${BASEADDRESS_AVIFIL32}" installbase="system32" installname="avifil32.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="avifil32.spec" />
	<include base="avifil32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>acmstream.c</file>
	<file>api.c</file>
	<file>avifile.c</file>
	<file>editstream.c</file>
	<file>extrachunk.c</file>
	<file>factory.c</file>
	<file>getframe.c</file>
	<file>icmstream.c</file>
	<file>regsvr.c</file>
	<file>tmpfile.c</file>
	<file>wavfile.c</file>
	<file>rsrc.rc</file>
	<file>avifil32.spec</file>
	<library>wine</library>
	<library>uuid</library>
	<library>msacm32</library>
	<library>msvfw32</library>
	<library>winmm</library>
	<library>ole32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
