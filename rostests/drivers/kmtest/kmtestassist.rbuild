<module name="kmtestassist" type="kernelmodedriver" installbase="system32/drivers" installname="kmtestassist.sys">
	<bootstrap base="$(CDOUTPUT)" />
	<define name="__USE_W32API" />
	<include base="ReactOS">include/reactos/drivers</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>kmtestassist.c</file>
</module>
