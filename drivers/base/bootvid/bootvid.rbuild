<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="bootvid" type="kernelmodedriver" installbase="system32/drivers" installname="bootvid.sys">
	<include base="bootvid">.</include>
        <define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>bootvid.c</file>
	<file>pixelsup_i386.S</file>
	<file>bootvid.rc</file>
</module>
</rbuild>
