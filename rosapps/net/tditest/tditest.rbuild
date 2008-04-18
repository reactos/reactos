<module name="tditest" type="kernelmodedriver" installbase="system32/drivers" installname="tditest.sys">
	<include base="tditest">include</include>
	<library>ntoskrnl</library>

	<directory name="tditest">
		<file>tditest.c</file>
	</directory>
</module>
