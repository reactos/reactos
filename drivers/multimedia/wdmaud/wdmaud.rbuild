<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="wdmaud_kernel" type="kernelmodedriver" installbase="system32/drivers" installname="wdmaud.sys" warnings="true">
	<include base="wdmaud">.</include>
	<include base="wdmaud">..</include>
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<file>entry.c</file>
</module>
</rbuild>
