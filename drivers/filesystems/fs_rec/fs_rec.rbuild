<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="fs_rec" type="kernelmodedriver" installbase="system32/drivers" installname="fs_rec.sys">
	<include base="fs_rec">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>blockdev.c</file>
	<file>cdfs.c</file>
	<file>ext2.c</file>
	<file>fat.c</file>
	<file>fs_rec.c</file>
	<file>ntfs.c</file>
	<file>udfs.c</file>
	<file>fs_rec.rc</file>
</module>
