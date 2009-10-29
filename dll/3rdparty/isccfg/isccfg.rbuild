<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<module name="isccfg" type="win32dll" installbase="system32" installname="libisccfg.dll" allowwarnings="true">
	<include base="ReactOS">dll/3rdparty/isc/include</include>
	<include base="ReactOS">dll/3rdparty/isc/win32</include>
	<include base="ReactOS">dll/3rdparty/isc/win32/include</include>
	<include base="ReactOS">dll/3rdparty/isc/noatomic/include</include>
	<include base="ReactOS">dll/3rdparty/dns/include</include>
	<include base="ReactOS">dll/3rdparty/dns/win32/include</include>
	<include base="ReactOS">dll/3rdparty/dns/sec/openssl/include</include>
	<include base="ReactOS">dll/3rdparty/isccfg/include</include>
	<include base="ReactOS">dll/3rdparty/isccfg/win32/include</include>
	<define name="WIN32" />
	<define name="USE_MD5" />
	<define name="LIBISCCFG_EXPORTS" />
	<define name="ISC_PLATFORM_HAVEIN6PKTINFO" />
	<define name="ISC_PLATFORM_USEGCCASM" />
	<file>aclconf.c</file>
	<file>log.c</file>
	<file>namedconf.c</file>
	<file>parser.c</file>
	<directory name="win32">
		<file>DLLMain.c</file>
		<file>version.c</file>
	</directory>
</module>