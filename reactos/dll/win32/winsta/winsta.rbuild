<module name="winsta" type="win32dll" baseaddress="${BASEADDRESS_WINSTA}" installbase="system32" installname="winsta.dll">
	<importlibrary definition="winsta.def" />
	<include base="winsta">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<library>ntdll</library>
	<pch>winsta.h</pch>
	<file>main.c</file>
	<file>misc.c</file>
	<file>server.c</file>
	<file>ws.c</file>
	<file>winsta.rc</file>
</module>
