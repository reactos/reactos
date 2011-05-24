<module name="netevent" type="win32dll" baseaddress="${BASEADDRESS_NETEVENT}" installbase="system32" installname="netevent.dll" entrypoint="0" unicode="true">
	<include base="netevent">.</include>
	<include base="neteventmsg" root="intermediate">.</include>
	<dependency>neteventmsg</dependency>
	<file>netevt.rc</file>
</module>
