<module name="mswsock" type="win32dll" baseaddress="${BASEADDRESS_MSWSOCK}" installbase="system32" installname="mswsock.dll" unicode="yes">
	<importlibrary definition="mswsock.spec"/>
	<include base="ReactOS">include/reactos/winsock</include>
	<include base="mswsock">dnslib/inc</include>
	<include base="ReactOS">include/reactos/drivers</include>
	<library>ntdll</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>ws2help</library>
	<library>ws2_32</library>
	<library>dnsapi</library>
	<directory name="mswsock">
		<file>init.c</file>
		<file>msext.c</file>
		<file>nspgaddr.c</file>
		<file>nspsvc.c</file>
		<file>nsptcpip.c</file>
		<file>nsputil.c</file>
		<file>proc.c</file>
		<file>recvex.c</file>
		<file>setup.c</file>
		<file>stubs.c</file>
	</directory>
	<directory name="dns">
		<file>addr.c</file>
		<file>debug.c</file>
		<file>dnsaddr.c</file>
		<file>dnsutil.c</file>
		<file>flatbuf.c</file>
		<file>hostent.c</file>
		<file>ip6.c</file>
		<file>memory.c</file>
		<file>name.c</file>
		<file>print.c</file>
		<file>record.c</file>
		<file>rrprint.c</file>
		<file>sablob.c</file>
		<file>straddr.c</file>
		<file>string.c</file>
		<file>table.c</file>
		<file>utf8.c</file>
	</directory>
	<directory name="msafd">
		<file>accept.c</file>
		<file>addrconv.c</file>
		<file>afdsan.c</file>
		<file>async.c</file>
		<file>bind.c</file>
		<file>connect.c</file>
		<file>eventsel.c</file>
		<file>getname.c</file>
		<file>helper.c</file>
		<file>listen.c</file>
		<file>nspeprot.c</file>
		<file>proc.c</file>
		<file>recv.c</file>
		<file>sanaccpt.c</file>
		<file>sanconn.c</file>
		<file>sanflow.c</file>
		<file>sanlistn.c</file>
		<file>sanprov.c</file>
		<file>sanrdma.c</file>
		<file>sanrecv.c</file>
		<file>sansend.c</file>
		<file>sanshutd.c</file>
		<file>sansock.c</file>
		<file>santf.c</file>
		<file>sanutil.c</file>
		<file>select.c</file>
		<file>send.c</file>
		<file>shutdown.c</file>
		<file>sockerr.c</file>
		<file>socket.c</file>
		<file>sockopt.c</file>
		<file>spi.c</file>
		<file>tpackets.c</file>
		<file>tranfile.c</file>
		<file>wspmisc.c</file>
	</directory>
	<directory name="rnr20">
		<file>context.c</file>
		<file>getserv.c</file>
		<file>init.c</file>
		<file>logit.c</file>
		<file>lookup.c</file>
		<file>nbt.c</file>
		<file>nsp.c</file>
		<file>oldutil.c</file>
		<file>proc.c</file>
		<file>r_comp.c</file>
		<file>util.c</file>
	</directory>
	<directory name="wsmobile">
		<file>lpc.c</file>
		<file>nsp.c</file>
		<file>service.c</file>
		<file>update.c</file>
	</directory>
	<file>mswsock.rc</file>
</module>
