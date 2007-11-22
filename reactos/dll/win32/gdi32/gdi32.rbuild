<module name="gdi32" type="win32dll" baseaddress="${BASEADDRESS_GDI32}" installbase="system32" installname="gdi32.dll">
	<importlibrary definition="gdi32.def" />
	<include base="gdi32">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="UNICODE" />
	<define name="WINVER">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>win32ksys</library>
	<library>pseh</library>

	<directory name="include">
		<pch>precomp.h</pch>
	</directory>
	<directory name="main">
		<file>dllmain.c</file>
	</directory>
	<directory name="misc">
		<file>heap.c</file>
		<file>gdientry.c</file>
		<file>hacks.c</file>
		<file>historic.c</file>
		<file>misc.c</file>
		<file>stubs.c</file>
		<file>stubsa.c</file>
		<file>stubsw.c</file>
		<file>wingl.c</file>
	</directory>
	<directory name="objects">
		<file>arc.c</file>
		<file>bitmap.c</file>
		<file>brush.c</file>
		<file>coord.c</file>
		<file>dc.c</file>
		<file>eng.c</file>
		<file>enhmfile.c</file>
		<file>font.c</file>
		<file>linedda.c</file>
		<file>metafile.c</file>
		<file>painting.c</file>
		<file>palette.c</file>
		<file>pen.c</file>
		<file>region.c</file>
		<file>text.c</file>
		<file>utils.c</file>
		<file>path.c</file>
	</directory>
	<linkerflag>-lgcc</linkerflag>
	<linkerflag>-nostartfiles</linkerflag>
	<linkerflag>-nostdlib</linkerflag>
	<file>gdi32.rc</file>
</module>
