<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="msfs" type="kernelmodedriver" installbase="system32/drivers" installname="msfs.sys">
	<include base="msfs">.</include>
        <define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>create.c</file>
	<file>finfo.c</file>
	<file>fsctrl.c</file>
	<file>msfs.c</file>
	<file>rw.c</file>
	<file>msfs.rc</file>
	<pch>msfs.h</pch>
</module>
</rbuild>
