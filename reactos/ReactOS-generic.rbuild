<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<xi:include href="baseaddress.rbuild" />

	<define name="__REACTOS__" />
	<define name="__REACTOS__" host="true" />

	<if property="DBG" value="1">
		<define name="DBG">1</define>
		<define name="_SEH_ENABLE_TRACE" />
		<property name="DBG_OR_KDBG" value="true" />
	</if>
	<if property="DBG" value="0">
		<define name="DBG">0</define>
	</if>

	<if property="KDBG" value="1">
		<define name="KDBG">1</define>
		<property name="DBG_OR_KDBG" value="true" />
	</if>

	<!-- The version target valid values are: Nt4 , NT5 , NT51 -->
	<property name="VERSION_TARGET" value="NT52" />

	<if property="VERSION_TARGET" value="NT4">
		<define name="WINVER">0x400</define>
		<define name="_WIN32_IE">0x600</define>
		<define name="_WIN32_WINNT">0x400</define>
		<define name="_WIN32_WINDOWS">0x400</define>
		<define name="_SETUPAPI_VER">0x400</define>
	</if>

	<if property="VERSION_TARGET" value="NT5">
		<define name="WINVER">0x500</define>
		<define name="_WIN32_IE">0x600</define>
		<define name="_WIN32_WINNT">0x500</define>
		<define name="_WIN32_WINDOWS">0x500</define>
		<define name="_SETUPAPI_VER">0x500</define>
	</if>

	<if property="VERSION_TARGET" value="NT51">
		<define name="WINVER">0x501</define>
		<define name="_WIN32_IE">0x600</define>
		<define name="_WIN32_WINNT">0x501</define>
		<define name="_WIN32_WINDOWS">0x501</define>
		<define name="_SETUPAPI_VER">0x501</define>
	</if>

	<if property="VERSION_TARGET" value="NT52">
		<define name="WINVER">0x502</define>
		<define name="_WIN32_IE">0x600</define>
		<define name="_WIN32_WINNT">0x502</define>
		<define name="_WIN32_WINDOWS">0x502</define>
		<define name="_SETUPAPI_VER">0x502</define>
	</if>

	<if property="VERSION_TARGET" value="NT6">
		<define name="WINVER">0x600</define>
		<define name="_WIN32_IE">0x600</define>
		<define name="_WIN32_WINNT">0x600</define>
		<define name="_WIN32_WINDOWS">0x600</define>
		<define name="_SETUPAPI_VER">0x600</define>
	</if>

	<include>.</include>
	<include>include</include>
	<include root="intermediate">include</include>
	<include>include/psdk</include>
	<include root="intermediate">include/psdk</include>
	<include>include/dxsdk</include>
	<include root="intermediate">include/dxsdk</include>
	<include>include/crt</include>
	<include compilerset="gcc">include/crt/mingw32</include>
	<include compilerset="msc">include/crt/msc</include>
	<include>include/ddk</include>
	<include>include/GL</include>
	<include>include/ndk</include>
	<include>include/reactos</include>
	<include root="intermediate">include/reactos</include>
	<include root="intermediate">include/reactos/mc</include>
	<include>include/reactos/libs</include>

	<include host="true">include</include>
	<include host="true" root="intermediate">include</include>
	<include host="true">include/reactos</include>
	<include host="true">include/reactos/wine</include>

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
		<if property="BUILD_MP" value="1">
			<xi:include href="ntoskrnl/ntkrnlmp.rbuild" />
		</if>
	</directory>
	<directory name="subsystems">
		<xi:include href="subsystems/subsystems.rbuild" />
	</directory>
	<directory name="tools">
		<xi:include href="tools/tools.rbuild" />
	</directory>

	<compilerflag compiler="cxx" compilerset="gcc">-Wno-non-virtual-dtor</compilerflag>
</group>
