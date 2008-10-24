<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dxdiagn" type="win32dll" baseaddress="${BASEADDRESS_DXDIAGN}" installbase="system32" installname="dxdiagn.dll" allowwarnings="true" unicode="yes">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="dxdiagn.spec" />
	<include base="dxdiagn">.</include>
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
	<library>oleaut32</library>
	<library>dxguid</library>
	<library>strmiids</library>
	<file>container.c</file>
	<file>dxdiag_main.c</file>
	<file>provider.c</file>
	<file>regsvr.c</file>
</module>
