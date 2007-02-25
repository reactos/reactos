<module name="pcnet" type="kernelmodedriver" installbase="system32/drivers" installname="pcnet.sys">
	<include base="pcnet">.</include>
	<define name="NDIS50_MINIPORT" />
	<define name="NDIS_MINIPORT_DRIVER" />
	<define name="NDIS_LEGACY_MINIPORT" />
	<define name="NDIS51_MINIPORT" />
	<define name="__USE_W32API" />
	<library>ndis</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>pcnet.c</file>
	<file>requests.c</file>
	<file>pcnet.rc</file>
</module>
