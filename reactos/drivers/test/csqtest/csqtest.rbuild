<module name="csqtest" type="kernelmodedriver" installbase="system32/drivers" installname="csqtest.sys">
	<bootstrap base="$(CDOUTPUT)" />
	<define name="__USE_W32API" />
	<include base="ReactOS">include/reactos/drivers</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>csqtest.c</file>
	<file>csqtest.rc</file>
</module>
