<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="kbdclass" type="kernelmodedriver" installbase="system32/drivers" installname="kbdclass.sys">
	<bootstrap base="reactos" />
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>kbdclass.c</file>
	<file>misc.c</file>
	<file>kbdclass.rc</file>
</module>
</rbuild>
