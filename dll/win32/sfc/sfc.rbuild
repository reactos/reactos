<module name="sfc" type="win32dll" baseaddress="${BASEADDRESS_SFC}" installbase="system32" installname="sfc.dll" allowwarnings="yes">
	<importlibrary definition="sfc.spec" />
	<include base="sfc">.</include>
	<file>sfc.c</file>
	<pch>precomp.h</pch>
</module>
