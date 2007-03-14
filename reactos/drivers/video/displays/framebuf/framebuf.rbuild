<module name="framebuf" type="kernelmodedll" entrypoint="_DrvEnableDriver@12" installbase="system32" installname="framebuf.dll">
	<importlibrary definition="framebuf.def" />
	<include base="framebuf">.</include>
	<define name="__USE_W32API" />
	<library>win32k</library>
	<library>libcntpr</library>
	<file>enable.c</file>
	<file>palette.c</file>
	<file>pointer.c</file>
	<file>screen.c</file>
	<file>surface.c</file>
	<file>framebuf.rc</file>
</module>
