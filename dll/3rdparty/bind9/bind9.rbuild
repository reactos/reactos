<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<module name="bind9" type="win32dll" installbase="system32" installname="libbind9.dll" allowwarnings="true">
	<include base="ReactOS">dll/3rdparty/isc/include</include>
	<include base="ReactOS">dll/3rdparty/isc/win32</include>
	<include base="ReactOS">dll/3rdparty/isc/win32/include</include>
	<include base="ReactOS">dll/3rdparty/isc/noatomic/include</include>
	<include base="ReactOS">dll/3rdparty/dns/include</include>
	<include base="ReactOS">dll/3rdparty/isccfg/include</include>
	<include base="ReactOS">dll/3rdparty/bind9/include</include>
	<include base="ReactOS">dll/3rdparty/bind9/win32/include</include>
	<define name="WIN32" />
	<define name="USE_MD5" />
	<define name="LIBBIND9_EXPORTS" />
	<define name="ISC_PLATFORM_HAVEIN6PKTINFO" />
	<define name="ISC_PLATFORM_USEGCCASM" />
	<library>isc</library>
	<library>dns</library>
	<library>isccfg</library>
	<library>ws2_32</library>
	<file>check.c</file>
	<file>getaddresses.c</file>
	<importlibrary definition="win32/libbind9.def" />
	<directory name="win32">
		<file>DLLMain.c</file>
		<file>version.c</file>
	</directory>
</module>