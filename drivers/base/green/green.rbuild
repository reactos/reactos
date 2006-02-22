<module name="green" type="kernelmodedriver" installbase="system32/drivers" installname="green.sys">
	<bootstrap base="reactos" />
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>createclose.c</file>
	<file>dispatch.c</file>
	<file>green.c</file>
	<file>keyboard.c</file>
	<file>misc.c</file>
	<file>pnp.c</file>
	<file>power.c</file>
	<file>screen.c</file>
	<file>green.rc</file>
	<pch>green.h</pch>
</module>
