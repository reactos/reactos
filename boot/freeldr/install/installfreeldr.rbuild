<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="installfreeldr" type="win32cui">
	<include base="installfreeldr">.</include>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<file>install.c</file>
	<file>volume.c</file>
</module>
</rbuild>
