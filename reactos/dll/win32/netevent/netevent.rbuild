<module name="netevent" type="win32dll" baseaddress="${BASEADDRESS_NETEVENT}" installbase="system32" installname="netevent.dll" unicode="true">
	<importlibrary definition="netevent.spec" />
	<include base="netevent">.</include>
	<include base="neteventmsg" root="intermediate">.</include>
	<dependency>neteventmsg</dependency>
	<file>netevent.c</file>
	<file>netevt.rc</file>
</module>
