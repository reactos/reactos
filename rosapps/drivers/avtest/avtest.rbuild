<module name="avtest" type="kernelmodedriver" installbase="system32/drivers" installname="avtest.sys">
	<include base="avtest">.</include>
	<define name="_NTDDK_" />
	<library>ks</library>
	<library>ntoskrnl</library>
	<file>entry.c</file>
</module>
