<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dplayx" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_DPLAYX}" installbase="system32" installname="dplayx.dll" allowwarnings ="true">
	<!-- Won't load correctly in ReactOS yet autoregister infsection="OleControlDlls" type="DllRegisterServer" -->
	<importlibrary definition="dplayx.spec.def" />
	<include base="dplayx">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
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
	<file>dpclassfactory.c</file>
	<file>dplay.c</file>
	<file>dplaysp.c</file>
	<file>dplayx_global.c</file>
	<file>dplayx_main.c</file>
	<file>dplayx_messages.c</file>
	<file>dplobby.c</file>
	<file>lobbysp.c</file>
	<file>name_server.c</file>
	<file>regsvr.c</file>
	<file>dplayx.spec</file>
</module>
