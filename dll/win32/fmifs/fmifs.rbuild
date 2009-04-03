<module name="fmifs" type="win32dll" entrypoint="InitializeFmIfs@12" baseaddress="${BASEADDRESS_FMIFS}" installbase="system32" installname="fmifs.dll">
	<importlibrary definition="fmifs.spec" />
	<include base="fmifs">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>ntdll</library>
	<library>kernel32</library>
	<file>chkdsk.c</file>
	<file>compress.c</file>
	<file>diskcopy.c</file>
	<file>extend.c</file>
	<file>format.c</file>
	<file>init.c</file>
	<file>media.c</file>
	<file>query.c</file>
	<file>fmifs.rc</file>
	<pch>precomp.h</pch>
</module>
