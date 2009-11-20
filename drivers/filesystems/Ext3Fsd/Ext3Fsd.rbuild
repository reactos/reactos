<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ext3fsd" type="kernelmodedriver" installbase="system32/drivers" installname="Ext3Fsd.sys" allowwarnings="true" entrypoint="DriverEntry">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="ext3fsd">.</include>
	<include base="ext3fsd">include</include>
	<define name="__KERNEL__" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>pseh</library>
	<library>libcntpr</library>
	<file>init.c</file>
	<file>block.c</file>
	<file>cleanup.c</file>
	<file>close.c</file>
	<file>cmcb.c</file>
	<file>create.c</file>
	<file>debug.c</file>
	<file>devctl.c</file>
	<file>dirctl.c</file>
	<file>dispatch.c</file>
	<file>except.c</file>
	<file>fastio.c</file>
	<file>fileinfo.c</file>
	<file>flush.c</file>
	<file>fsctl.c</file>
	<file>linux.c</file>
	<file>lock.c</file>
	<file>memory.c</file>
	<file>misc.c</file>
	<file>nls.c</file>
	<file>pnp.c</file>
	<file>read.c</file>
	<file>shutdown.c</file>
	<file>volinfo.c</file>
	<file>write.c</file>
	<file>Ext3Fsd.rc</file>
	<pch>include/ext2fs.h</pch>
	<directory name="ext3">
	    <file>recover.c</file>
	    <file>generic.c</file>
	</directory>
	<directory name="jbd">
	    <file>recovery.c</file>
        <file>revoke.c</file>
        <file>replay.c</file>
    </directory>
    <directory name="nls">
		<file>nls_cp1251.c</file>
		<file>nls_cp861.c</file>
		<file>nls_iso8859-13.c</file>
		<file>nls_cp737.c</file>
		<file>nls_iso8859-6.c</file>
		<file>nls_cp857.c</file>
		<file>nls_base.c</file>
		<file>nls_cp863.c</file>
		<file>nls_utf8.c</file>
		<file>nls_koi8-ru.c</file>
		<file>nls_ascii.c</file>
		<file>nls_koi8-u.c</file>
		<file>nls_iso8859-2.c</file>
		<file>nls_cp775.c</file>
		<file>nls_cp864.c</file>
		<file>nls_iso8859-4.c</file>
		<file>nls_cp437.c</file>
		<file>nls_cp862.c</file>
		<file>nls_cp949.c</file>
		<file>nls_iso8859-15.c</file>
		<file>nls_cp850.c</file>
		<file>nls_koi8-r.c</file>
		<file>nls_iso8859-14.c</file>
		<file>nls_iso8859-5.c</file>
		<file>nls_cp936.c</file>
		<file>nls_euc-jp.c</file>
		<file>nls_cp852.c</file>
		<file>nls_cp1250.c</file>
		<file>nls_iso8859-1.c</file>
		<file>nls_cp855.c</file>
		<file>nls_iso8859-9.c</file>
		<file>nls_iso8859-7.c</file>
		<file>nls_cp950.c</file>
		<file>nls_cp874.c</file>
		<file>nls_iso8859-3.c</file>
		<file>nls_cp865.c</file>
		<file>nls_cp869.c</file>
		<file>nls_cp932.c</file>
		<file>nls_cp860.c</file>
		<file>nls_cp1255.c</file>
		<file>nls_cp866.c</file>
    </directory>
   	<compilerflag compilerset="gcc">-fms-extensions</compilerflag>
</module>
