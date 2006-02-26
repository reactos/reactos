<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="vbemp" type="kernelmodedriver" installbase="system32/drivers" installname="vbemp.sys">
	<include base="vbemp">.</include>
	<define name="__USE_W32API" />
	<library>videoprt</library>
	<file>vbemp.c</file>
	<file>vbemp.rc</file>
</module>
</rbuild>
