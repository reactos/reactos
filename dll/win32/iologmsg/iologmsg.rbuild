<module name="iologmsg" type="win32dll" baseaddress="${BASEADDRESS_IOLOGMSG}" installbase="system32" installname="iologmsg.dll" entrypoint="0" unicode="true">
	<include base="iologmsg">.</include>
	<include base="ntiologc" root="intermediate">.</include>
	<dependency>ntiologc</dependency>
	<file>iologmsg.rc</file>
</module>
