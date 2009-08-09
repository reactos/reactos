<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ext2fs" type="kernelmodedriver" installbase="system32/drivers" installname="ext2.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="ext2fs">inc</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<directory name="src">
		<file>cleanup.c</file>
		<file>close.c</file>
		<file>create.c</file>
		<file>devcntrl.c</file>
		<file>dircntrl.c</file>
		<file>DiskIO.c</file>
		<file>ext2init.c</file>
		<file>fastio.c</file>
		<file>fileinfo.c</file>
		<file>flush.c</file>
		<file>fsctrl.c</file>
		<file>io.c</file>
		<file>metadata.c</file>
		<file>misc.c</file>
		<file>read.c</file>
		<file>shutdown.c</file>
		<file>volinfo.c</file>
		<file>write.c</file>
	</directory>
	<directory name="inc">
		<pch>ext2fsd.h</pch>
	</directory>
	<file>ext2fs.rc</file>
</module>
