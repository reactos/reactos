<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="npfs" type="kernelmodedriver" installbase="system32/drivers" installname="npfs.sys">
	<include base="npfs">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>pseh</library>
	<file>create.c</file>
	<file>dirctl.c</file>
	<file>finfo.c</file>
	<file>fsctrl.c</file>
	<file>npfs.c</file>
	<file>rw.c</file>
	<file>volume.c</file>
	<file>npfs.rc</file>
	<pch>npfs.h</pch>
</module>
