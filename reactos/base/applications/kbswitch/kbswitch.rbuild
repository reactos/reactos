<?xml version="1.0"?>
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="kbswitch" type="win32gui" installbase="system32" installname="kbswitch.exe" unicode="yes">
		<include base="kbswitch">.</include>
		<define name="_WIN32_IE">0x600</define>
		<define name="WINVER">0x500</define>
		<define name="_WIN32_WINNT">0x501</define>
		<library>kernel32</library>
		<library>advapi32</library>
		<library>ntdll</library>
		<library>user32</library>
		<library>gdi32</library>
		<library>shell32</library>
		<library>comctl32</library>
		<library>msimg32</library>
		<library>shlwapi</library>
        <file>kbswitch.c</file>
		<file>kbswitch.rc</file>
	</module>
</group>