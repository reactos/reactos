<module name="kmtest" type="kernelmodedriver" installbase="system32/drivers" installname="kmtest.sys">
	<bootstrap base="$(CDOUTPUT)" />
	<define name="__USE_W32API" />
	<include base="ReactOS">include/reactos/drivers</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>kmtest.c</file>
	<file>deviface.c</file>
	<file>deviface_test.c</file>
	<file>ntos_ex.c</file>
	<file>ntos_io.c</file>
	<file>ntos_ob.c</file>
	<file>kmtest.rc</file>
</module>
