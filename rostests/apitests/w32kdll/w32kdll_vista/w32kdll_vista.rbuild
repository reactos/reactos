<module name="w32kdll_vista" type="win32dll" entrypoint="0" installname="w32kdll_vista.dll">
	<importlibrary definition="w32kdll_vista.def" />
	<define name="__USE_W32API" />
	<file>w32kdll_vista.S</file>
	<file>main.c</file>
</module>
