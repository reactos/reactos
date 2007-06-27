<module name="mouclass" type="kernelmodedriver" installbase="system32/drivers" installname="mouclass.sys">
	<include base="mouclass">.</include>
	<define name="__USE_W32API" />
	<define name="NDEBUG" />
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>misc.c</file>
	<file>mouclass.c</file>
	<file>mouclass.rc</file>
</module>
