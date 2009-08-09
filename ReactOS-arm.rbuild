<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile-arm.auto" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xi:include href="config-arm.rbuild">
		<xi:fallback>
			<xi:include href="config-arm.template.rbuild" />
		</xi:fallback>
	</xi:include>

	<xi:include href="baseaddress.rbuild" />

	<define name="__REACTOS__" />
	<define name="_ARM_" />
	<define name="__arm__" />
	<define name="TARGET_arm" host="true" />

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
	<include>include/reactos/arm</include>

	<property name="WINEBUILD_FLAGS" value="--kill-at"/>
	<property name="NTOSKRNL_SHARED" value="-file-alignment=0x1000 -section-alignment=0x1000 -shared"/>

	<if property="SARCH" value="versatile">
		<define name="BOARD_CONFIG_VERSATILE"/>
	</if>

	<if property="OPTIMIZE" value="1">
		<compilerflag>-Os</compilerflag>
		<compilerflag>-ftracer</compilerflag>
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

	<define name="__MSVCRT__"/>
	<compilerflag>-Wno-attributes</compilerflag>
	<compilerflag>-fno-strict-aliasing</compilerflag>
	<linkerflag>--strip-debug</linkerflag>
	<linkerflag>-static</linkerflag>
	
	<directory name="media">
		<directory name="nls">
			<xi:include href="media/nls/nls.rbuild" />
		</directory>	
	</directory>
	<directory name="lib">
		<directory name="drivers">
			<directory name="csq">
				<xi:include href="lib/drivers/csq/csq.rbuild" />
            </directory>
		</directory>
		<directory name="debugsup">
			<xi:include href="lib/debugsup/debugsup.rbuild" />
		</directory>
		<directory name="3rdparty">
			<directory name="zlib">
				<xi:include href="lib/3rdparty/zlib/zlib.rbuild" />
			</directory>
		</directory>
		<directory name="rtl">
			<xi:include href="lib/rtl/rtl.rbuild" />
		</directory>
		<directory name="host">
			<directory name="wcsfuncs">
				<xi:include href="lib/host/wcsfuncs/wcsfuncs.rbuild" />
			</directory>
		</directory>
		<directory name="inflib">
			<xi:include href="lib/inflib/inflib.rbuild" />
		</directory>
		<directory name="cmlib">
			<xi:include href="lib/cmlib/cmlib.rbuild" />
		</directory>
		<directory name="pseh">
			<xi:include href="lib/pseh/pseh.rbuild" />
		</directory>
		<directory name="rossym">
			<xi:include href="lib/rossym/rossym.rbuild" />
		</directory>
		<directory name="sdk">
			<directory name="crt">
				<xi:include href="lib/sdk/crt/crt.rbuild" />
				<xi:include href="lib/sdk/crt/libcntpr.rbuild" />
			</directory>
			<directory name="nt">
				<xi:include href="lib/sdk/nt/nt.rbuild" />
			</directory>
			<directory name="wdmguid">
				<xi:include href="lib/sdk/wdmguid/wdmguid.rbuild" />
			</directory>
		</directory>
		<directory name="ntdllsys">
			<xi:include href="lib/ntdllsys/ntdllsys.rbuild" />
		</directory>
		<directory name="smlib">
			<xi:include href="lib/smlib/smlib.rbuild" />
		</directory>
	</directory>
	<directory name="include">
		<xi:include href="include/directory.rbuild" />
	</directory>
	<directory name="tools">
		<xi:include href="tools/tools.rbuild" />
	</directory>
	<directory name="ntoskrnl">
		<xi:include href="ntoskrnl/ntoskrnl.rbuild" />
	</directory>
	<directory name="hal">
		<directory name="halarm">
			<directory name="generic">
				<xi:include href="hal/halarm/generic/generic.rbuild" />
			</directory>
			<directory name="up">
				<xi:include href="hal/halarm/up/halup.rbuild" />
			</directory>
		</directory>
		<directory name="hal">
			<xi:include href="hal/hal/hal.rbuild" />
		</directory>
	</directory>
	<directory name="boot">
		<xi:include href="boot/boot.rbuild" />
	</directory>
	<directory name="drivers">
		<directory name="storage">
			<directory name="class">
				<directory name="ramdisk">
					<xi:include href="drivers/storage/class/ramdisk/ramdisk.rbuild" />
				</directory>
			</directory>
		</directory>
		<directory name="filesystems">
			<directory name="fastfat">
				<xi:include href="drivers/filesystems/fastfat/vfatfs.rbuild" />
			</directory>
		</directory>
		<directory name="base">
			<directory name="kdcom">
				<xi:include href="drivers/base/kdcom/kdcom.rbuild" />
			</directory>
			<directory name="bootvid">
				<xi:include href="drivers/base/bootvid/bootvid.rbuild" />
			</directory>
		</directory>
	</directory>
	<directory name="dll">
		<directory name="ntdll">
			<xi:include href="dll/ntdll/ntdll.rbuild" />
		</directory>	
	</directory>
	<directory name="base">
		<directory name="system">
            <directory name="smss">
                <xi:include href="base/system/smss/smss.rbuild" />
            </directory>	
		</directory>	
	</directory>
</project>
