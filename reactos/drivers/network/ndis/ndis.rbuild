<module name="ndis" type="exportdriver" installbase="system32/drivers" installname="ndis.sys">
	<importlibrary definition="ndis.def"></importlibrary>
	<include base="ndis">include</include>
	<define name="NDIS_WRAPPER" />
	<define name="NDIS50" />
	<define name="NDIS51" />
	<define name="NDIS50_MINIPORT" />
	<define name="NDIS51_MINIPORT" />
	<define name="NDIS_LEGACY_DRIVER" />
	<define name="NDIS_LEGACY_MINIPORT" />
	<define name="NDIS_LEGACY_PROTOCOL" />
	<define name="NDIS_MINIPORT_DRIVER" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<directory name="include">
		<pch>ndissys.h</pch>
	</directory>
	<directory name="ndis">
		<file>40gone.c</file>
		<file>50gone.c</file>
		<file>buffer.c</file>
		<file>cl.c</file>
		<file>cm.c</file>
		<file>co.c</file>
		<file>config.c</file>
		<file>control.c</file>
		<file>efilter.c</file>
		<file>hardware.c</file>
		<file>io.c</file>
		<file>main.c</file>
		<file>memory.c</file>
		<file>miniport.c</file>
		<file>protocol.c</file>
		<file>string.c</file>
		<file>stubs.c</file>
		<file>time.c</file>
	</directory>
	<file>ndis.rc</file>
</module>
