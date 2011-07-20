<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kernel32" type="win32dll" crt="dll" baseaddress="${BASEADDRESS_KERNEL32}" installbase="system32" installname="kernel32.dll">
	<importlibrary definition="kernel32.pspec" />
	<include base="kernel32">.</include>
	<include base="kernel32" root="intermediate">.</include>
	<include base="kernel32">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<library>pseh</library>
	<library>normalize</library>
	<library>ntdll</library>
	<define name="_KERNEL32_" />
	<redefine name="_WIN32_WINNT">0x0600</redefine>
	<dependency>errcodes</dependency>
	<pch>k32.h</pch>
	<group compilerset="gcc">
		<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
		<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	</group>
	<directory name="debug">
		<file>debugger.c</file>
	</directory>
	<directory name="except">
		<file>except.c</file>
	</directory>
	<directory name="file">
		<file>backup.c</file>
		<file>bintype.c</file>
		<file>cnotify.c</file>
		<file>copy.c</file>
		<file>console.c</file>
		<file>create.c</file>
		<file>curdir.c</file>
		<file>delete.c</file>
		<file>deviceio.c</file>
		<file>dir.c</file>
		<file>dosdev.c</file>
		<file>file.c</file>
		<file>filemap.c</file>
		<file>find.c</file>
		<file>hardlink.c</file>
		<file>iocompl.c</file>
		<file>lfile.c</file>
		<file>lock.c</file>
		<file>mailslot.c</file>
		<file>move.c</file>
		<file>npipe.c</file>
		<file>pipe.c</file>
		<file>rw.c</file>
		<file>tape.c</file>
		<file>volume.c</file>
	</directory>
	<directory name="mem">
		<file>heap.c</file>
		<file>virtual.c</file>
	</directory>
	<directory name="misc">
		<file>actctx.c</file>
		<file>atom.c</file>
		<file>comm.c</file>
		<file>computername.c</file>
		<file>dllmain.c</file>
		<file>env.c</file>
		<file>handle.c</file>
		<file>ldr.c</file>
		<file>lzexpand.c</file>
		<file>muldiv.c</file>
		<file>perfcnt.c</file>
		<file>power.c</file>
		<file>resntfy.c</file>
		<file>res.c</file>
		<file>stubs.c</file>
		<file>sysinfo.c</file>
		<file>time.c</file>
		<file>timerqueue.c</file>
		<file>toolhelp.c</file>
		<file>version.c</file>
		<file>profile.c</file>
		<file>utils.c</file>
	</directory>
	<directory name="process">
		<file>job.c</file>
		<file>proc.c</file>
		<file>session.c</file>
	</directory>
	<directory name="string">
		<file>chartype.c</file>
		<file>collation.c</file>
		<file>casemap.c</file>
		<file>fold.c</file>
		<file>format_msg.c</file>
		<file>lang.c</file>
		<file>lstring.c</file>
		<file>lcformat.c</file>
		<file>nls.c</file>
		<file>sortkey.c</file>
	</directory>
	<directory name="vista">
		<file>vista.c</file>
	</directory>
	<directory name="synch">
		<file>synch.c</file>
	</directory>
	<directory name="thread">
		<file>fiber.c</file>
		<file>thread.c</file>
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>fiber.S</file>
				<file>thread.S</file>
			</directory>
		</if>
		<if property="ARCH" value="amd64">
			<directory name="amd64">
				<file>fiber.S</file>
				<file>thread.S</file>
			</directory>
		</if>
	</directory>
	<file>kernel32.rc</file>
</module>
