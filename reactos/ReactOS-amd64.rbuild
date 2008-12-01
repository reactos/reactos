<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile-amd64.auto" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xi:include href="config-amd64.rbuild">
		<xi:fallback>
			<xi:include href="config-amd64.template.rbuild" />
		</xi:fallback>
	</xi:include>

	<define name="_M_AMD64" />
	<define name="_AMD64_" />
	<define name="_M_AXP64" />
	<define name="__x86_64__" />
	<define name="_X86AMD64_" />
	<define name="_WIN64" />
	<define name="TARGET_amd64" host="true" />

	<property name="PLATFORM" value="PC"/>
	<property name="usewrc" value="false"/>
	<property name="WINEBUILD_FLAGS" value="--kill-at"/>
	<property name="NTOSKRNL_SHARED" value="-shared"/>

	<if property="OPTIMIZE" value="1">
		<compilerflag>-Os</compilerflag>
		<compilerflag>-ftracer</compilerflag>
		<compilerflag>-momit-leaf-frame-pointer</compilerflag>
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

	<compilerflag>-U_X86_</compilerflag>
	<compilerflag>-mpreferred-stack-boundary=4</compilerflag>
	<compilerflag>-fno-strict-aliasing</compilerflag>
	<compilerflag>-Wno-strict-aliasing</compilerflag>
	<compilerflag>-Wpointer-arith</compilerflag>
	<linkerflag>-disable-stdcall-fixup</linkerflag>
	<linkerflag>-static</linkerflag>
	<linkerflag>--unique=.eh_frame</linkerflag>

<!-- Here starts <xi:include href="ReactOS-generic.rbuild" /> -->

	<xi:include href="baseaddress.rbuild" />

	<define name="__REACTOS__" />
	<if property="DBG" value="1">
		<define name="DBG">1</define>
		<define name="_SEH_ENABLE_TRACE" />
		<property name="DBG_OR_KDBG" value="true" />
	</if>
	<if property="KDBG" value="1">
		<define name="KDBG">1</define>
		<property name="DBG_OR_KDBG" value="true" />
	</if>

	<!-- The version target valid values are: Nt4 , NT5 , NT51 -->
	<property name="VERSION_TARGET" value="NT52" />

	<if property="VERSION_TARGET" value="NT4">
		<define name="WINVER" overridable="true">0x400</define>
		<define name="_WIN32_IE">0x600</define>
		<define name="_WIN32_WINNT" overridable="true">0x400</define>
		<define name="_WIN32_WINDOWS">0x400</define>
		<define name="_SETUPAPI_VER">0x400</define>
	</if>

	<if property="VERSION_TARGET" value="NT5">
		<define name="WINVER" overridable="true">0x500</define>
		<define name="_WIN32_IE">0x600</define>
		<define name="_WIN32_WINNT" overridable="true">0x500</define>
		<define name="_WIN32_WINDOWS">0x500</define>
		<define name="_SETUPAPI_VER">0x500</define>
	</if>

	<if property="VERSION_TARGET" value="NT51">
		<define name="WINVER" overridable="true">0x501</define>
		<define name="_WIN32_IE">0x600</define>
		<define name="_WIN32_WINNT" overridable="true">0x501</define>
		<define name="_WIN32_WINDOWS">0x501</define>
		<define name="_SETUPAPI_VER">0x501</define>
	</if>

	<if property="VERSION_TARGET" value="NT52">
		<define name="WINVER" overridable="true">0x502</define>
		<define name="_WIN32_IE">0x600</define>
		<define name="_WIN32_WINNT" overridable="true">0x502</define>
		<define name="_WIN32_WINDOWS">0x502</define>
		<define name="_SETUPAPI_VER">0x502</define>
	</if>

	<if property="VERSION_TARGET" value="NT6">
		<define name="WINVER" overridable="true">0x600</define>
		<define name="_WIN32_IE">0x600</define>
		<define name="_WIN32_WINNT" overridable="true">0x600</define>
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
	<include>include/crt/mingw32</include>
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

	<!-- directory name="base">
		<xi:include href="base/base.rbuild" />
	</directory -->

	<directory name="boot">
		<xi:include href="boot/boot.rbuild" />
	</directory>

	<!-- directory name="dll">
		<xi:include href="dll/dll.rbuild" />
	</directory -->

	<directory name="drivers">
		<directory name="bus">
			<directory name="pci">
				<xi:include href="drivers/bus/pci/pci.rbuild" />
			</directory>
		</directory>

		<!-- directory name="directx">
			<xi:include href="drivers/directx/directory.rbuild" />
		</directory -->

		<!-- directory name="ksfilter">
			<xi:include href="drivers/ksfilter/directory.rbuild" />
		</directory -->

		<!-- directory name="multimedia">
			<xi:include href="drivers/multimedia/directory.rbuild" />
		</directory -->

		<directory name="parallel">
			<xi:include href="drivers/parallel/directory.rbuild" />
		</directory>

		<directory name="serial">
			<xi:include href="drivers/serial/directory.rbuild" />
		</directory>

		<!--directory name="usb">	
			<xi:include href="drivers/usb/directory.rbuild" />
		</directory-->
	
		<!--directory name="video">
			<xi:include href="drivers/video/directory.rbuild" />
		</directory-->
	
		<!-- directory name="wdm">
			<xi:include href="drivers/wdm/wdm.rbuild" />
		</directory -->

		<directory name="wmi">
			<xi:include href="drivers/wmi/wmilib.rbuild" />
		</directory>

		<directory name="base">
			<xi:include href="drivers/base/directory.rbuild" />
		</directory>
		<directory name="filesystems">
			<xi:include href="drivers/filesystems/directory.rbuild" />
		</directory>
		<directory name="input">
			<xi:include href="drivers/input/directory.rbuild" />
		</directory>
		<directory name="network">
			<xi:include href="drivers/network/directory.rbuild" />
		</directory>
		<directory name="setup">
			<xi:include href="drivers/setup/directory.rbuild" />
		</directory>
		<directory name="storage">
			<directory name="class">
				<directory name="cdrom">
					<xi:include href="drivers/storage/class/cdrom/cdrom.rbuild" />
				</directory>
				<directory name="class2">
					<xi:include href="drivers/storage/class/class2/class2.rbuild" />
				</directory>
				<directory name="disk">
					<xi:include href="drivers/storage/class/disk/disk.rbuild" />
				</directory>
			</directory>
			<directory name="floppy">
				<xi:include href="drivers/storage/floppy/floppy.rbuild" />
			</directory>
			<directory name="ide">
				<directory name="atapi">
					<xi:include href="drivers/storage/ide/atapi/atapi.rbuild" />
				</directory>
			</directory>
			<directory name="port">
				<xi:include href="drivers/storage/port/directory.rbuild" />
			</directory>
			<directory name="scsiport">
				<xi:include href="drivers/storage/scsiport/scsiport.rbuild" />
			</directory>
		</directory>
	</directory>

	<directory name="hal">
		<xi:include href="hal/hal.rbuild" />
	</directory>

	<directory name="include">
		<xi:include href="include/directory.rbuild" />
	</directory>

	<directory name="lib">
		<directory name="3rdparty">
			<directory name="adns">
				<xi:include href="lib/3rdparty/adns/adns.rbuild" />
			</directory>
			<directory name="bzip2">
				<xi:include href="lib/3rdparty/bzip2/bzip2.rbuild" />
			</directory>
			<directory name="expat">
				<xi:include href="lib/3rdparty/expat/expat.rbuild" />
			</directory>
			<directory name="icu4ros">
				<xi:include href="lib/3rdparty/icu4ros/icu4ros.rbuild" />
			</directory>
			<directory name="libwine">
				<xi:include href="lib/3rdparty/libwine/libwine.rbuild" />
			</directory>
			<directory name="libxml2">
				<xi:include href="lib/3rdparty/libxml2/libxml2.rbuild" />
			</directory>
			<!--directory name="mingw">
				<xi:include href="lib/3rdparty/mingw/mingw.rbuild" />
			</directory-->
			<directory name="zlib">
				<xi:include href="lib/3rdparty/zlib/zlib.rbuild" />
			</directory>
		</directory>
		<directory name="sdk">
			<xi:include href="lib/sdk/sdk.rbuild" />
		</directory>
		<directory name="cmlib">
			<xi:include href="lib/cmlib/cmlib.rbuild" />
		</directory>
		<directory name="debugsup">
			<xi:include href="lib/debugsup/debugsup.rbuild" />
		</directory>
		<directory name="drivers">
			<xi:include href="lib/drivers/directory.rbuild" />
		</directory>
		<directory name="epsapi">
			<xi:include href="lib/epsapi/epsapi.rbuild" />
		</directory>
		<directory name="fslib">
			<xi:include href="lib/fslib/directory.rbuild" />
		</directory>
		<directory name="host">
			<xi:include href="lib/host/directory.rbuild" />
		</directory>
		<directory name="inflib">
			<xi:include href="lib/inflib/inflib.rbuild" />
		</directory>
		<directory name="nls">
			<xi:include href="lib/nls/nls.rbuild" />
		</directory>
		<directory name="ntdllsys">
			<xi:include href="lib/ntdllsys/ntdllsys.rbuild" />
		</directory>
		<directory name="pseh">
			<xi:include href="lib/pseh/pseh.rbuild" />
		</directory>
		<directory name="recyclebin">
			<xi:include href="lib/recyclebin/recyclebin.rbuild" />
		</directory>
		<directory name="rossym">
			<xi:include href="lib/rossym/rossym.rbuild" />
		</directory>
		<directory name="rtl">
			<xi:include href="lib/rtl/rtl.rbuild" />
		</directory>
		<directory name="smlib">
			<xi:include href="lib/smlib/smlib.rbuild" />
		</directory>
		<directory name="win32ksys">
			<xi:include href="lib/win32ksys/win32ksys.rbuild" />
		</directory>
	</directory>

	<directory name="media">
		<xi:include href="media/media.rbuild" />
	</directory>

	<directory name="ntoskrnl">
		<xi:include href="ntoskrnl/ntoskrnl.rbuild" />
	</directory>

	<!-- directory name="subsystems">
		<xi:include href="subsystems/subsystems.rbuild" />
	</directory -->

	<directory name="tools">
		<xi:include href="tools/tools.rbuild" />
	</directory>

<!-- Here ends <xi:include href="ReactOS-generic.rbuild" /> -->

</project>
