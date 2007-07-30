<module name="diskdump" type="kernelmodedriver" installbase="system32/drivers" installname="diskdump.sys">
	<bootstrap base="$(CDOUTPUT)" />
	<define name="__USE_W32API" />
	<include base="ReactOS">include/reactos/drivers</include>
	<library definition="diskdump.def" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>class2</library>
	<include base="diskdump">..</include>
	<file>diskdump.c</file>
	<file>diskdump_helper.S</file>
	<file>diskdump.rc</file>
</module>
