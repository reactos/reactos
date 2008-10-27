<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<group>
	<module name="ntdll" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_NTDLL}" installbase="system32" installname="ntdll.dll">
		<bootstrap installbase="$(CDOUTPUT)/system32" />
		<importlibrary definition="def/ntdll_$(ARCH).def" />
		<include base="ntdll">include</include>
		<include base="ntdll" root="intermediate"></include>
		<include base="ReactOS">include/reactos/subsys</include>
		<define name="__NTDLL__" />
		<define name="_DISABLE_TIDENTS" />
		<define name="_NTOSKRNL_" />
		<define name="__NO_CTYPE_INLINES" />
		<library>rtl</library>
		<library>ntdllsys</library>
		<library>libcntpr</library>
		<library>pseh</library>
		<dependency>ntstatus</dependency>
		<directory name="csr">
			<file>api.c</file>
			<file>capture.c</file>
			<file>connect.c</file>
		</directory>
		<directory name="dbg">
			<file>dbgui.c</file>
		</directory>
		<directory name="dispatch">
			<if property="ARCH" value="i386">
				<directory name="i386">
					<file>dispatch.S</file>
				</directory>
			</if>
			<if property="ARCH" value="amd64">
				<directory name="amd64">
					<file>stubs.c</file>
				</directory>
			</if>
			<if property="ARCH" value="arm">
				<directory name="arm">
					<file>stubs_asm.s</file>
				</directory>
			</if>
			<ifnot property="ARCH" value="i386">
				<file>dispatch.c</file>
			</ifnot>
		</directory>
		<directory name="include">
			<pch>ntdll.h</pch>
		</directory>
		<directory name="ldr">
			<file>startup.c</file>
			<file>utils.c</file>
		</directory>
		<directory name="rtl">
			<file>libsupp.c</file>
			<file>version.c</file>
		</directory>
		<directory name="def">
			<file>ntdll.rc</file>
		</directory>
	</module>
</group>
