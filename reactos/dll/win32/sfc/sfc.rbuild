<module name="sfc" type="win32dll" baseaddress="${BASEADDRESS_SFC}" installbase="system32" installname="sfc.dll">
	<importlibrary definition="sfc.spec" />
	<include base="sfc">.</include>
	<library>kernel32</library>
	<file>sfc.c</file>
	<pch>precomp.h</pch>
</module>
