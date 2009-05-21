<module name="ufatx" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_UFAT}" installbase="system32" installname="ufatx.dll">
	<importlibrary definition="ufatx.def" />
	<include base="ufatx">.</include>
	<library>vfatxlib</library>
	<library>ntdll</library>
	<file>ufatx.rc</file>
</module>
