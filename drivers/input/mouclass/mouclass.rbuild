<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="mouclass" type="kernelmodedriver" installbase="system32/drivers" installname="mouclass.sys">
	<include base="mouclass">.</include>
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>misc.c</file>
	<file>mouclass.c</file>
	<file>mouclass.rc</file>
</module>
</rbuild>
