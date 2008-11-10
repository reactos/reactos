<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="comcat" type="win32dll" baseaddress="${BASEADDRESS_COMCAT}" installbase="system32" entrypoint="0" installname="comcat.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="comcat.spec" />
	<include base="comcat">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>comcat_main.c</file>
	<file>factory.c</file>
	<file>information.c</file>
	<file>manager.c</file>
	<file>register.c</file>
	<file>regsvr.c</file>
	<file>version.rc</file>
</module>
