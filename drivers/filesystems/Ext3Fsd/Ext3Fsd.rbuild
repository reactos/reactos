<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ext3fsd" type="kernelmodedriver" installbase="system32/drivers" installname="Ext3Fsd.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="ext3fsd">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>pseh</library>
	<file>block.c</file>
	<file>cleanup.c</file>
	<file>close.c</file>
	<file>cmcb.c</file>
	<file>create.c</file>
	<file>debug.c</file>
	<file>devctl.c</file>
	<file>dirctl.c</file>
	<file>dispatch.c</file>
	<file>except.c</file>
	<file>fastio.c</file>
	<file>fileinfo.c</file>
	<file>flush.c</file>
	<file>fsctl.c</file>
	<file>init.c</file>
	<file>linux.c</file>
	<file>lock.c</file>
	<file>memory.c</file>
	<file>misc.c</file>
	<file>nls.c</file>
	<file>pnp.c</file>
	<file>read.c</file>
	<file>shutdown.c</file>
	<file>volinfo.c</file>
	<file>write.c</file>
	<file>Ext3Fsd.rc</file>
<!--	<pch>include/ext2fs.h</pch> -->
</module>
