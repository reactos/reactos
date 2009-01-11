<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="fastfatn" type="kernelmodedriver" installbase="system32/drivers" installname="fastfatn.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="fastfatn">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>blockdev.c</file>
	<file>cleanup.c</file>
	<file>close.c</file>
	<file>create.c</file>
	<file>dir.c</file>
	<file>direntry.c</file>
	<file>ea.c</file>
	<file>fastfat.c</file>
	<file>fat.c</file>
	<file>fastio.c</file>
	<file>fcb.c</file>
	<file>finfo.c</file>
	<file>flush.c</file>
	<file>fsctl.c</file>
	<file>rw.c</file>
	<file>shutdown.c</file>
	<file>volume.c</file>
	<file>fastfat.rc</file>
	<pch>fastfat.h</pch>
</module>
