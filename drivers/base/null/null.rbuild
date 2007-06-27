<module name="null" type="kernelmodedriver" installbase="system32/drivers" installname="null.sys">
	<include base="null">.</include>
	<define name="__USE_W32API" />
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>null.c</file>
	<file>null.rc</file>
</module>
