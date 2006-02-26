<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="null" type="kernelmodedriver" installbase="system32/drivers" installname="null.sys">
	<include base="null">.</include>
	<define name="__USE_W32API" />
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>null.c</file>
	<file>null.rc</file>
</module>
</rbuild>
