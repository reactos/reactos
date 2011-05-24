<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="fastfatn" type="kernelmodedriver" installbase="system32/drivers" installname="fastfatn.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<include base="fastfatn">.</include>
	<include base="ReactOS">include/reactos/libs/fullfat</include>
	<library>fullfat</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>pseh</library>
	<file>cleanup.c</file>
	<file>close.c</file>
	<file>create.c</file>
	<file>device.c</file>
	<file>dir.c</file>
	<file>ea.c</file>
	<file>fastfat.c</file>
	<file>fat.c</file>
	<file>fastio.c</file>
	<file>fcb.c</file>
	<file>finfo.c</file>
	<file>flush.c</file>
	<file>fsctl.c</file>
	<file>fullfat.c</file>
	<file>lock.c</file>
	<file>rw.c</file>
	<file>shutdown.c</file>
	<file>volume.c</file>
	<file>fastfat.rc</file>
	<pch>fastfat.h</pch>
</module>
