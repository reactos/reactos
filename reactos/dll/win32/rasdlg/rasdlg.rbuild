<module name="rasdlg" type="win32dll" baseaddress="${BASEADDRESS_RASDLG}" installbase="system32" installname="rasdlg.dll">
	<importlibrary definition="rasdlg.spec" />
	<include base="rasdlg">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>uuid</library>
	<file>rasdlg.c</file>
	<file>rasdlg.rc</file>
</module>
