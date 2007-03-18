<module name="kbdclass" type="kernelmodedriver" installbase="system32/drivers" installname="kbdclass.sys">
	<bootstrap base="reactos" />
	<define name="__USE_W32API" />
	<define name="NDEBUG" />
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>kbdclass.c</file>
	<file>misc.c</file>
	<file>kbdclass.rc</file>
</module>
