<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<xi:include href="baseaddress.rbuild" />
	<xi:include href="buildfamilies.rbuild" />
	<xi:include href="contributors.rbuild" />
	<xi:include href="installfolders.rbuild" />
	<xi:include href="languages.rbuild" />

	<define name="__REACTOS__" overridable="true" />
	<if property="MP" value="1">
		<define name="CONFIG_SMP">1</define>
	</if>
	<if property="DBG" value="1">
		<define name="DBG">1</define>
		<define name="_SEH_ENABLE_TRACE" />
		<property name="DBG_OR_KDBG" value="true" />
	</if>
	<if property="KDBG" value="1">
		<define name="KDBG">1</define>
		<property name="DBG_OR_KDBG" value="true" />
	</if>

	<include>.</include>
	<include>include</include>
	<include root="intermediate">include</include>
	<include>include/psdk</include>
	<include root="intermediate">include/psdk</include>
	<include>include/dxsdk</include>
	<include>include/crt</include>
	<include>include/crt/mingw32</include>
	<include>include/ddk</include>
	<include>include/GL</include>
	<include>include/ndk</include>
	<include>include/reactos</include>
	<include root="intermediate">include/reactos</include>
	<include>include/reactos/libs</include>

	<directory name="base">
		<xi:include href="base/base.rbuild" />
	</directory>
	<directory name="boot">
		<xi:include href="boot/boot.rbuild" />
	</directory>
	<directory name="dll">
		<xi:include href="dll/dll.rbuild" />
	</directory>
	<directory name="drivers">
		<xi:include href="drivers/drivers.rbuild" />
	</directory>
	<directory name="hal">
		<xi:include href="hal/hal.rbuild" />
	</directory>
	<directory name="include">
		<xi:include href="include/directory.rbuild" />
	</directory>
	<directory name="lib">
		<xi:include href="lib/lib.rbuild" />
	</directory>
	<directory name="media">
		<xi:include href="media/media.rbuild" />
	</directory>
	<directory name="modules">
		<xi:include href="modules/directory.rbuild" />
	</directory>
	<directory name="ntoskrnl">
		<xi:include href="ntoskrnl/ntoskrnl.rbuild" />
	</directory>
	<directory name="subsystems">
		<xi:include href="subsystems/subsystems.rbuild" />
	</directory>

	<!-- The languages to be included in to the platform being build. see languages.rbuild for additional languages -->
	<platformlanguage isoname="en-US" />
	<platformlanguage isoname="es-ES" />
	<platformlanguage isoname="de-DE" />

</group>
