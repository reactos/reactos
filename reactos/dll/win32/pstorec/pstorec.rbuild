<module name="pstorec" type="win32dll" baseaddress="${BASEADDRESS_PSTOREC}" installbase="system32" installname="pstorec.dll">
	<importlibrary definition="pstorec.spec" />
	<include base="pstorec">.</include>
	<library>wine</library>
	<library>kernel32</library>
	<library>uuid</library>
	<file>pstorec.c</file>
</module>
