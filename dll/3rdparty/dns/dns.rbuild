<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dns" type="win32dll" installbase="system32" installname="libdns.dll" allowwarnings="true">
	<include base="ReactOS">dll/3rdparty/dns/include</include>
	<include base="ReactOS">dll/3rdparty/dns/win32/include</include>
	<include base="ReactOS">dll/3rdparty/isc/include</include>
	<include base="ReactOS">dll/3rdparty/isc/win32</include>
	<include base="ReactOS">dll/3rdparty/isc/win32/include</include>
	<include base="ReactOS">dll/3rdparty/isc/noatomic/include</include>
	<define name="WIN32" />
	<define name="USE_MD5" />
	<define name="LIBDNS_EXPORTS" />
	<define name="ISC_PLATFORM_HAVEIN6PKTINFO" />
	<define name="ISC_PLATFORM_USEGCCASM" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>uuid</library>
	<library>ws2_32</library>
	<library>msvcrt40</library>
	<library>isc</library>
	<importlibrary definition="win32/libdns.def" />
	<directory name="win32">
		<file>DLLMain.c</file>
		<file>version.c</file>
	</directory>
	<file>acache.c</file>
	<file>acl.c</file>
	<file>adb.c</file>
	<file>byaddr.c</file>
	<file>cache.c</file>
	<file>callbacks.c</file>
	<file>compress.c</file>
	<file>db.c</file>
	<file>dbiterator.c</file>
	<file>dbtable.c</file>
	<file>diff.c</file>
	<file>dispatch.c</file>
	<file>dlz.c</file>
	<file>dnssec.c</file>
	<file>ds.c</file>
	<file>dst_api.c</file>
	<file>dst_lib.c</file>
	<file>dst_parse.c</file>
	<file>dst_result.c</file>
	<file>forward.c</file>
	<file>gssapi_link.c</file>
	<file>gssapictx.c</file>
	<file>hmac_link.c</file>
	<file>iptable.c</file>
	<file>journal.c</file>
	<file>key.c</file>
	<file>keytable.c</file>
	<file>lib.c</file>
	<file>log.c</file>
	<file>lookup.c</file>
	<file>master.c</file>
	<file>masterdump.c</file>
	<file>message.c</file>
	<file>name.c</file>
	<file>ncache.c</file>
	<file>nsec.c</file>
	<file>nsec3.c</file>
	<file>openssl_link.c</file>
	<file>openssldh_link.c</file>
	<file>openssldsa_link.c</file>
	<file>opensslrsa_link.c</file>
	<file>order.c</file>
	<file>peer.c</file>
	<file>portlist.c</file>
	<file>rbt.c</file>
	<file>rbtdb.c</file>
	<file>rbtdb64.c</file>
	<file>rcode.c</file>
	<file>rdata.c</file>
	<file>rdatalist.c</file>
	<file>rdataset.c</file>
	<file>rdatasetiter.c</file>
	<file>rdataslab.c</file>
	<file>request.c</file>
	<file>resolver.c</file>
	<file>result.c</file>
	<file>rootns.c</file>
	<file>sdb.c</file>
	<file>sdlz.c</file>
	<file>soa.c</file>
	<file>spnego.c</file>
	<file>ssu.c</file>
	<file>stats.c</file>
	<file>tcpmsg.c</file>
	<file>time.c</file>
	<file>timer.c</file>
	<file>tkey.c</file>
	<file>tsig.c</file>
	<file>ttl.c</file>
	<file>validator.c</file>
	<file>view.c</file>
	<file>xfrin.c</file>
	<file>zone.c</file>
	<file>zonekey.c</file>
	<file>zt.c</file>
</module>