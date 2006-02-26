<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="blue" type="kernelmodedriver" installbase="system32/drivers" installname="blue.sys">
	<bootstrap base="reactos" />
        <define name="__USE_W32API" />
	<include base="ReactOS">include/reactos/drivers</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>blue.c</file>
	<file>blue.rc</file>
</module>
</rbuild>
