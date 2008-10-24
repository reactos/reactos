<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dinput8" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_DINPUT8}" installbase="system32" installname="dinput8.dll" unicode="yes">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="dinput8.spec" />
	<include base="dinput8">.</include>
	<include base="ReactOS">include/reactos/wine</include>
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
	<library>dinput</library>
	<file>version.rc</file>
	<file>dinput8_main.c</file>
</module>
