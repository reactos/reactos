<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="eventvwr" type="win32gui" installbase="system32" installname="eventvwr.exe" allowwarnings="true">
	<include base="eventvwr">.</include>
	<define name="__REACTOS__" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>advapi32</library>
	<file>eventvwr.c</file>
	<file>eventvwr.rc</file>
</module>
</rbuild>
