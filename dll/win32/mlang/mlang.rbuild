<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="mlang" type="win32dll" baseaddress="${BASEADDRESS_MLANG}" installbase="system32" installname="mlang.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="mlang.spec" />
	<include base="mlang">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<file>mlang.c</file>
	<file>regsvr.c</file>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>oleaut32</library>
	<library>uuid</library>
	<library>ntdll</library>
</module>
</group>
