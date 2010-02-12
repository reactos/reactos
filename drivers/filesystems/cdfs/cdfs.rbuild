<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="cdfs" type="kernelmodedriver" installbase="system32/drivers" installname="cdfs.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="cdfs">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>cdfs.c</file>
	<file>cleanup.c</file>
	<file>close.c</file>
	<file>common.c</file>
	<file>create.c</file>
	<file>dirctl.c</file>
	<file>fcb.c</file>
	<file>finfo.c</file>
	<file>fsctl.c</file>
	<file>misc.c</file>
	<file>rw.c</file>
	<file>volinfo.c</file>
	<file>cdfs.rc</file>
	<pch>cdfs.h</pch>
</module>
