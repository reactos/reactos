<module name="msafd" type="win32dll" baseaddress="${BASEADDRESS_MSAFD}" installbase="system32" installname="msafd.dll">
	<importlibrary definition="msafd.def" />
	<include base="msafd">.</include>
	<include base="msafd">include</include>
	<include base="ReactOS">include/reactos/drivers</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<pch>msafd.h</pch>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<directory name="misc">
		<file>dllmain.c</file>
		<file>event.c</file>
		<file>helpers.c</file>
		<file>sndrcv.c</file>
		<file>stubs.c</file>
	</directory>
	<file>msafd.rc</file>
</module>
