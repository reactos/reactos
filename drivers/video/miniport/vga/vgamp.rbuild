<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="vgamp" type="kernelmodedriver" installbase="system32/drivers" installname="vgamp.sys">
	<include base="vgamp">.</include>
	<define name="__USE_W32API" />
	<library>videoprt</library>
	<file>initvga.c</file>
	<file>vgamp.c</file>
	<file>vgamp.rc</file>
	<pch>vgamp.h</pch>
</module>
</rbuild>
