<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dplay" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_DPLAY}" installbase="system32" installname="dplay.dll" unicode="yes">
	<importlibrary definition="dplay.spec" />
	<include base="dinput8">.</include>
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
	<library>dinput</library>
	<file>version.rc</file>
	<file>dplay_main.c</file>
</module>
