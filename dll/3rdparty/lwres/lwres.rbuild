<module name="lwres" type="win32dll" installbase="system32" installname="liblwres.dll" allowwarnings="true">
	<include base="ReactOS">dll/3rdparty/isc/include</include>
	<include base="ReactOS">dll/3rdparty/isc/win32</include>
	<include base="ReactOS">dll/3rdparty/isc/win32/include</include>
	<include base="ReactOS">dll/3rdparty/isc/noatomic/include</include>
	<include base="ReactOS">dll/3rdparty/lwres/win32/include/lwres</include>
	<include base="ReactOS">dll/3rdparty/lwres/include</include>
	<include base="ReactOS">dll/3rdparty/lwres/win32/include</include>
	<include base="ReactOS">dll/3rdparty/dns/include</include>
	<include base="ReactOS">dll/3rdparty/dns/win32/include</include>
	<include base="ReactOS">dll/3rdparty/dns/sec/openssl/include</include>
	<define name="WIN32" />
	<define name="USE_MD5" />
	<define name="LIBLWRES_EXPORTS" />
	<define name="ISC_PLATFORM_HAVEIN6PKTINFO" />
	<define name="ISC_PLATFORM_USEGCCASM" />
	<file>context.c</file>
	<file>gai_strerror.c</file>
	<file>getaddrinfo.c</file>
	<file>gethost.c</file>
	<file>getipnode.c</file>
	<file>getnameinfo.c</file>
	<file>getrrset.c</file>
	<file>herror.c</file>
	<file>lwbuffer.c</file>
	<file>lwinetaton.c</file>
	<file>lwinetntop.c</file>
	<file>lwinetpton.c</file>
	<file>lwpacket.c</file>
	<file>lwres_gabn.c</file>
	<file>lwres_gnba.c</file>
	<file>lwres_grbn.c</file>
	<file>lwres_noop.c</file>
	<file>lwresutil.c</file>
	<directory name="win32">
		<file>lwconfig.c</file>
		<file>DLLMain.c</file>
		<file>version.c</file>
		<file>socket.c</file>
	</directory>
</module>