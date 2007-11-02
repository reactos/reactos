<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ntfs" type="kernelmodedriver" installbase="system32/drivers" installname="ntfs.sys">
	<bootstrap />
	<include base="ntfs">.</include>
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>attrib.c</file>
	<file>blockdev.c</file>
	<file>close.c</file>
	<file>create.c</file>
	<file>dirctl.c</file>
	<file>fcb.c</file>
	<file>finfo.c</file>
	<file>fsctl.c</file>
	<file>mft.c</file>
	<file>ntfs.c</file>
	<file>rw.c</file>
	<file>volinfo.c</file>
	<file>ntfs.rc</file>
	<pch>ntfs.h</pch>
</module>
