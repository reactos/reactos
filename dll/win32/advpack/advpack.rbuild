<module name="advpack" type="win32dll" baseaddress="${BASEADDRESS_ADVPACK}"  installbase="system32" installname="advpack.dll">
	<importlibrary definition="advpack.def" />
	<include base="advpack">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>version</library>
	<file>advpack.c</file>
	<file>stubs.c</file>
	<file>advpack.rc</file>
</module>
