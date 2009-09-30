<module name="kmtestassist" type="kernelmodedriver" installbase="system32/drivers" installname="kmtestassist.sys">
	<bootstrap base="$(CDOUTPUT)" />
	<include base="ReactOS">include/reactos/drivers</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>kmtestassist.c</file>
</module>
