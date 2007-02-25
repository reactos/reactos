<module name="vgaddi" type="kernelmodedll" entrypoint="_DrvEnableDriver@12" installbase="system32" installname="vgaddi.dll">
	<importlibrary definition="vgaddi.def" />
	<include base="vgaddi">.</include>
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>win32k</library>
	<directory name="main">
		<file>enable.c</file>
	</directory>
	<directory name="objects">
		<file>screen.c</file>
		<file>pointer.c</file>
		<file>lineto.c</file>
		<file>paint.c</file>
		<file>bitblt.c</file>
		<file>transblt.c</file>
		<file>offscreen.c</file>
		<file>copybits.c</file>
	</directory>
	<directory name="vgavideo">
		<file>vgavideo.c</file>
	</directory>
	<file>vgaddi.rc</file>
</module>
