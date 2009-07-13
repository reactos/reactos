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

	<group compilerset="gcc">
		<compilerflag>-Wall</compilerflag>
		<compilerflag compiler="cxx">-Wno-non-virtual-dtor</compilerflag>
		<compilerflag compiler="cc,cxx">-gstabs+</compilerflag>
		<compilerflag compiler="as">-gstabs+</compilerflag>
	</group>

	<group compilerset="msc">
		<define name="inline" compiler="cc">__inline</define>
		<compilerflag>/Zl</compilerflag>
		<compilerflag>/Zi</compilerflag>
		<compilerflag>/W1</compilerflag>
	</group>

	<group compilerset="gcc">
		<if property="OPTIMIZE" value="1">
			<compilerflag>-Os</compilerflag>
		</if>
		<if property="OPTIMIZE" value="2">
			<compilerflag>-Os</compilerflag>
		</if>
		<if property="OPTIMIZE" value="3">
			<compilerflag>-O1</compilerflag>
		</if>
		<if property="OPTIMIZE" value="4">
			<compilerflag>-O2</compilerflag>
		</if>
		<if property="OPTIMIZE" value="5">
			<compilerflag>-O3</compilerflag>
		</if>

		<compilerflag>-fno-strict-aliasing</compilerflag>
		<compilerflag>-Wno-strict-aliasing</compilerflag>
		<compilerflag>-Wpointer-arith</compilerflag>
		<compilerflag>-Wno-multichar</compilerflag>
		<!-- compilerflag>-H</compilerflag>    enable this for header traces -->
	</group>

	<group compilerset="msc">
		<if property="OPTIMIZE" value="1">
			<compilerflag>/O1</compilerflag>
		</if>
		<if property="OPTIMIZE" value="2">
			<compilerflag>/O2</compilerflag>
		</if>
		<if property="OPTIMIZE" value="3">
			<compilerflag>/Ox /GS-</compilerflag>
			<compilerflag>/Ot</compilerflag>
		</if>
		<if property="OPTIMIZE" value="4">
			<compilerflag>/Ox /GS-</compilerflag>
			<compilerflag>/Os</compilerflag>
		</if>
		<if property="OPTIMIZE" value="5">
			<compilerflag>/Ox /GS-</compilerflag>
			<compilerflag>/Os</compilerflag>
			<compilerflag>/Ob2</compilerflag>
			<compilerflag>/GF</compilerflag>
			<compilerflag>/Gy</compilerflag>
		</if>

		<compilerflag>/GS-</compilerflag>
	</group>

	<define name="_USE_32BIT_TIME_T" />
</group>
