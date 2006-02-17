<module name="gdiplus" type="win32dll" baseaddress="${BASEADDRESS_GDIPLUS}" installbase="system32" installname="gdiplus.dll">
	<importlibrary definition="gdiplus.def" />
	<include base="gdiplus">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>gdi32</library>
	<library>msvcrt</library>
	<directory name="gdiplus">
		<compilationunit name="unit.c">
			<file>arrow.c</file>
			<file>bitmap.c</file>
			<file>brush.c</file>
			<file>clip.c</file>
			<file>codec.c</file>
			<file>container.c</file>
			<file>dllmain.c</file>
			<file>draw.c</file>
			<file>effect.c</file>
			<file>fill.c</file>
			<file>font.c</file>
			<file>graphics.c</file>
			<file>image.c</file>
			<file>linecap.c</file>
			<file>linegradient.c</file>
			<file>matrix.c</file>
			<file>memory.c</file>
			<file>metafile.c</file>
			<file>palette.c</file>
			<file>path.c</file>
			<file>pathgradient.c</file>
			<file>pathiterator.c</file>
			<file>pen.c</file>
			<file>region.c</file>
			<file>string.c</file>
			<file>texture.c</file>
			<file>transform.c</file>
		</compilationunit>
	</directory>
	<file>gdiplus.rc</file>
</module>
