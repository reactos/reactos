<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kernel32" type="win32dll" crt="dll" baseaddress="${BASEADDRESS_KERNEL32}" installbase="system32" installname="kernel32.dll">
	<importlibrary definition="kernel32.pspec" />
	<include base="kernel32">.</include>
	<include base="kernel32" root="intermediate">.</include>
	<include base="kernel32">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<library>pseh</library>
	<library>wine</library>
	<library>ntdll</library>
	<define name="_KERNEL32_" />
	<dependency>errcodes</dependency>
	<pch>k32.h</pch>
	<directory name="client">
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
		<file>appcache.c</file>
		<file>atom.c</file>
		<file>compname.c</file>
		<file>debugger.c</file>
		<file>dllmain.c</file>
		<file>dosdev.c</file>
		<file>environ.c</file>
		<file>except.c</file>
		<file>fiber.c</file>
		<file>handle.c</file>
		<file>heapmem.c</file>
		<file>job.c</file>
		<file>loader.c</file>
		<file>path.c</file>
		<file>perfcnt.c</file>
		<file>power.c</file>
		<file>proc.c</file>
		<file>resntfy.c</file>
		<file>session.c</file>
		<file>synch.c</file>
		<file>sysinfo.c</file>
		<file>time.c</file>
		<file>timerqueue.c</file>
		<file>thread.c</file>
		<file>toolhelp.c</file>
		<file>utils.c</file>
		<file>vdm.c</file>
		<file>version.c</file>
		<file>virtmem.c</file>
		<file>vista.c</file>
		<directory name="file">
			<file>backup.c</file>
			<file>cnotify.c</file>
			<file>copy.c</file>
			<file>console.c</file>
			<file>create.c</file>
			<file>delete.c</file>
			<file>deviceio.c</file>
			<file>dir.c</file>
			<file>fileinfo.c</file>
			<file>filemap.c</file>
			<file>filename.c</file>
			<file>find.c</file>
			<file>hardlink.c</file>
			<file>iocompl.c</file>
			<file>lfile.c</file>
			<file>lock.c</file>
			<file>mailslot.c</file>
			<file>move.c</file>
			<file>npipe.c</file>
			<file>rw.c</file>
			<file>tape.c</file>
			<file>volume.c</file>
		</directory>
	</directory>
	<directory name="wine">
		<file>actctx.c</file>
		<file>comm.c</file>
		<file>lzexpand.c</file>
		<file>muldiv.c</file>
		<file>profile.c</file>
		<file>res.c</file>
		<file>timezone.c</file>
	</directory>
	<directory name="winnls">
		<directory name="string">
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
	</directory>
	<file>kernel32.rc</file>
</module>
