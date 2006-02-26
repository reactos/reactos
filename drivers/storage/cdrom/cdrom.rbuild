<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="cdrom" type="kernelmodedriver" installbase="system32/drivers" installname="cdrom.sys">
	<bootstrap base="reactos" />
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>class2</library>
	<include base="cdrom">..</include>
	<file>cdrom.c</file>
	<file>cdrom.rc</file>
</module>
</rbuild>
