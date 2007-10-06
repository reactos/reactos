<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="devenum" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_DEVENUM}" installbase="system32" installname="devenum.dll" allowwarnings="true">
	<!-- Won't load correctly in ReactOS yet autoregister infsection="OleControlDlls" type="DllRegisterServer" -->
	<importlibrary definition="devenum.spec.def" />
	<include base="devenum">.</include>
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
	<library>advapi32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>winmm</library>
	<library>dxguid</library>
	<library>strmiids</library>
	<library>user32</library>
	<file>devenum.rc</file>
	<file>createdevenum.c</file>
	<file>devenum_main.c</file>
	<file>factory.c</file>
	<file>mediacatenum.c</file>
	<file>parsedisplayname.c</file>
	<file>devenum.spec</file>
</module>
