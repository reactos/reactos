<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dinput" type="win32dll" baseaddress="${BASEADDRESS_DINPUT}" installbase="system32" installname="dinput.dll" allowwarnings ="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="dinput.spec.def" />
	<include base="dinput">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
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
	<file>data_formats.c</file>
	<file>device.c</file>
	<file>dinput_main.c</file>
	<file>effect_linuxinput.c</file>
	<file>joystick_linux.c</file>
	<file>joystick_linuxinput.c</file>
	<file>keyboard.c</file>
	<file>mouse.c</file>
	<file>regsvr.c</file>
	<file>dinput.spec</file>
</module>
