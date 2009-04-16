<module name="w32kdll_xpsp2" type="win32dll" entrypoint="0" installname="w32kdll_xpsp2.dll">
	<importlibrary definition="w32kdll_xpsp2.def" />
	<define name="__USE_W32API" />
	<file>w32kdll_xpsp2.S</file>
	<file>main.c</file>
</module>
