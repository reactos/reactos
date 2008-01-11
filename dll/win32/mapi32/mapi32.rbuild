<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mapi32" type="win32dll" baseaddress="${BASEADDRESS_MAPI32}" installbase="system32" installname="mapi32.dll" allowwarnings="true">
	<importlibrary definition="mapi32.spec.def" />
	<include base="mapi32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>shlwapi</library>
	<library>shell32</library>
	<library>ole32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>uuid</library>
	<file>imalloc.c</file>
	<file>mapi32_main.c</file>
	<file>prop.c</file>
	<file>sendmail.c</file>
	<file>util.c</file>
	<file>mapi32.spec</file>
</module>
