<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="input" type="win32dll" extension=".dll" baseaddress="${BASEADDRESS_INPUT}" installbase="system32" installname="input.dll" unicode="yes">
	<importlibrary definition="input.spec" />
	<include base="input">.</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>gdi32</library>
	<file>input.c</file>
	<file>settings.c</file>
	<file>keysettings.c</file>
	<file>add.c</file>
	<file>changekeyseq.c</file>
	<file>input.rc</file>
	<file>input.spec</file>
</module>
