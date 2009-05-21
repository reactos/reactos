<module name="ws2_32_new" type="win32dll" baseaddress="${BASEADDRESS_WS2_32}" installbase="system32" installname="ws2_32.dll" unicode="yes">
	<importlibrary definition="ws2_32.spec" />
	<include base="ws2_32">include</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="LE" />
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>dnsapi</library>
	<directory name="src">
		<file>addrconv.c</file>
		<file>addrinfo.c</file>
		<file>async.c</file>
		<file>bhook.c</file>
		<file>dcatalog.c</file>
		<file>dcatitem.c</file>
		<file>dllmain.c</file>
		<file>dprocess.c</file>
		<file>dprovide.c</file>
		<file>dsocket.c</file>
		<file>dthread.c</file>
		<file>dupsock.c</file>
		<file>enumprot.c</file>
		<file>event.c</file>
    <file>getproto.c</file>
    <file>getxbyxx.c</file>
    <file>ioctl.c</file>
    <file>nscatalo.c</file>
    <file>nscatent.c</file>
    <file>nspinstl.c</file>
    <file>nsprovid.c</file>
    <file>nsquery.c</file>
    <file>qos.c</file>
    <file>qshelpr.c</file>
    <file>rasdial.c</file>
    <file>recv.c</file>
    <file>rnr.c</file>
    <file>scihlpr.c</file>
    <file>select.c</file>
    <file>send.c</file>
    <file>sockctrl.c</file>
    <file>socklife.c</file>
    <file>spinstal.c</file>
    <file>sputil.c</file>
    <file>startup.c</file>
    <file>wsautil.c</file>
	</directory>
	<file>ws2_32.rc</file>
</module>

