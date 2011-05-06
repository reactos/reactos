<module name="csqtest" type="kernelmodedriver" installbase="system32/drivers" installname="csqtest.sys">
	<bootstrap base="$(CDOUTPUT)" />
	<include base="ReactOS">include/reactos/drivers</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>csqtest.c</file>
	<file>csqtest.rc</file>
</module>
