<module name="ncftp" type="win32cui" installbase="system32" installname="ncftp.exe" allowwarnings="true">
	<include base="ncftp">.</include>
	<include base="ncftp">sio</include>
	<include base="ncftp">Strn</include>
	<include base="ncftp">libncftp</include>
	<include base="ncftp">ncftp</include>

	<define name="ncftp" />
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>ws2_32</library>

	<directory name="sio">
		<file>PRead.c</file>
		<file>PWrite.c</file>
		<file>SAcceptA.c</file>
		<file>SAcceptS.c</file>
		<file>SBind.c</file>
		<file>SClose.c</file>
		<file>SConnect.c</file>
		<file>SConnectByName.c</file>
		<file>SNew.c</file>
		<file>SRead.c</file>
		<file>SReadline.c</file>
		<file>SRecv.c</file>
		<file>SRecvfrom.c</file>
		<file>SRecvmsg.c</file>
		<file>SSelect.c</file>
		<file>SSend.c</file>
		<file>SSendto.c</file>
		<file>SSendtoByName.c</file>
		<file>SWrite.c</file>
		<file>SocketUtil.c</file>
		<file>StrAddr.c</file>
		<file>UAcceptA.c</file>
		<file>UAcceptS.c</file>
		<file>UBind.c</file>
		<file>UConnect.c</file>
		<file>UConnectByName.c</file>
		<file>UNew.c</file>
		<file>URecvfrom.c</file>
		<file>USendto.c</file>
		<file>USendtoByName.c</file>
		<file>SError.c</file>
		<file>SWait.c</file>
		<file>main.c</file>
	</directory>

	<directory name="Strn">
		<file>Dynscat.c</file>
		<file>Strncpy.c</file>
		<file>Strncat.c</file>
		<file>Strntok.c</file>
		<file>Strnpcpy.c</file>
		<file>Strnpcat.c</file>
		<file>strtokc.c</file>
		<file>version.c</file>
	</directory>

	<directory name="libncftp">
		<file>open.c</file>
		<file>cmds.c</file>
		<file>util.c</file>
		<file>rcmd.c</file>
		<file>ftp.c</file>
		<file>io.c</file>
		<file>errno.c</file>
		<file>linelist.c</file>
		<file>glob.c</file>
	</directory>

	<directory name="ncftp">
		<file>cmds.c</file>
		<file>cmdlist.c</file>
		<file>getopt.c</file>
		<file>ls.c</file>
		<file>main.c</file>
		<file>version.c</file>
		<file>shell.c</file>
		<file>util.c</file>
		<file>readln.c</file>
		<file>progress.c</file>
		<file>bookmark.c</file>
		<file>pref.c</file>
		<file>preffw.c</file>
		<file>trace.c</file>
		<file>spool.c</file>
		<file>log.c</file>
		<file>getline.c</file>
	</directory>
</module>
