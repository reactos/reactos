<module name="shfolder" type="win32dll" baseaddress="${BASEADDRESS_SHFOLDER}" installbase="system32" installname="shfolder.dll">
	<importlibrary definition="shfolder.spec.def" />
	<include base="shfolder">.</include>
	<include base="shfolder">include</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<library>shell32</library>
	<file>shfolder_main.c</file>
	<file>shfolder.spec</file>
</module>
