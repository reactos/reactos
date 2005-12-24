<module name="npfs" type="kernelmodedriver" installbase="system32/drivers" installname="npfs.sys">
	<include base="npfs">.</include>
	<define name="__USE_W32API" />
	<define name="__NO_CTYPE_INLINES" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>create.c</file>
	<file>finfo.c</file>
	<file>fsctrl.c</file>
	<file>npfs.c</file>
	<file>rw.c</file>
	<file>volume.c</file>
	<file>npfs.rc</file>
	<pch>npfs.h</pch>
</module>
