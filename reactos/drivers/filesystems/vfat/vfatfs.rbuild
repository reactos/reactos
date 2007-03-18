<module name="vfatfs" type="kernelmodedriver" installbase="system32/drivers" installname="vfatfs.sys">
	<bootstrap base="$(CDOUTPUT)" />
	<include base="vfatfs">.</include>
	<define name="__USE_W32API" />
	<linkerflag>-lgcc</linkerflag>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>blockdev.c</file>
	<file>cleanup.c</file>
	<file>close.c</file>
	<file>create.c</file>
	<file>dir.c</file>
	<file>direntry.c</file>
	<file>dirwr.c</file>
	<file>ea.c</file>
	<file>fat.c</file>
	<file>fastio.c</file>
	<file>fcb.c</file>
	<file>finfo.c</file>
	<file>flush.c</file>
	<file>fsctl.c</file>
	<file>iface.c</file>
	<file>misc.c</file>
	<file>rw.c</file>
	<file>shutdown.c</file>
	<file>string.c</file>
	<file>volume.c</file>
	<file>vfatfs.rc</file>
	<pch>vfat.h</pch>
</module>
