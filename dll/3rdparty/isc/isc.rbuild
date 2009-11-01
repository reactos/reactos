<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="isc" type="win32dll" installbase="system32" installname="libisc.dll" allowwarnings="true">
	<include base="ReactOS">dll/3rdparty/isc</include>
	<include base="ReactOS">dll/3rdparty/isc/include</include>
	<include base="ReactOS">dll/3rdparty/isc/win32</include>
	<include base="ReactOS">dll/3rdparty/isc/win32/include</include>
	<include base="ReactOS">dll/3rdparty/isc/noatomic/include</include>
	<include base="ReactOS">dll/3rdparty/isccfg/include</include>
	<define name="WIN32" />
	<define name="LIBISC_EXPORTS" />
	<define name="ISC_PLATFORM_HAVEIN6PKTINFO" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>uuid</library>
	<library>ws2_32</library>
	<library>msvcrt40</library>
	<file>assertions.c</file>
	<file>base32.c</file>
	<file>base64.c</file>
	<file>bitstring.c</file>
	<file>buffer.c</file>
	<file>bufferlist.c</file>
	<file>commandline.c</file>
	<file>error.c</file>
	<file>event.c</file>
	<file>hash.c</file>
	<file>heap.c</file>
	<file>hex.c</file>
	<file>hmacmd5.c</file>
	<file>hmacsha.c</file>
	<file>httpd.c</file>
	<file>inet_aton.c</file>
	<file>inet_ntop.c</file>
	<file>inet_pton.c</file>
	<file>iterated_hash.c</file>
	<file>lex.c</file>
	<file>lfsr.c</file>
	<file>lib.c</file>
	<file>log.c</file>
	<file>md5.c</file>
	<file>mem.c</file>
	<file>mutexblock.c</file>
	<file>netaddr.c</file>
	<file>netscope.c</file>
	<file>ondestroy.c</file>
	<file>parseint.c</file>
	<file>portset.c</file>
	<file>quota.c</file>
	<file>radix.c</file>
	<file>random.c</file>
	<file>ratelimiter.c</file>
	<file>refcount.c</file>
	<file>region.c</file>
	<file>result.c</file>
	<file>rwlock.c</file>
	<file>serial.c</file>
	<file>sha1.c</file>
	<file>sha2.c</file>
	<file>sockaddr.c</file>
	<file>stats.c</file>
	<file>string.c</file>
	<file>strtoul.c</file>
	<file>symtab.c</file>
	<file>task.c</file>
	<file>taskpool.c</file>
	<file>timer.c</file>
	<importlibrary definition="win32/libisc.def" />
	<directory name="nls">
		<file>msgcat.c</file>
	</directory>
	<directory name="win32">
		<file>app.c</file>
		<file>condition.c</file>
		<file>dir.c</file>
		<file>DLLMain.c</file>
		<file>entropy.c</file>
		<file>errno2result.c</file>
		<file>file.c</file>
		<file>fsaccess.c</file>
		<file>interfaceiter.c</file>
		<file>ipv6.c</file>
		<file>keyboard.c</file>
		<file>net.c</file>
		<file>ntpaths.c</file>
		<file>once.c</file>
		<file>os.c</file>
		<file>resource.c</file>
		<file>socket.c</file>
		<file>stdio.c</file>
		<file>stdtime.c</file>
		<file>strerror.c</file>
		<file>syslog.c</file>
		<file>thread.c</file>
		<file>time.c</file>
		<file>version.c</file>
		<file>win32os.c</file>
	</directory>
</module>