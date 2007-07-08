<module name="ddrawex" type="win32dll" entrypoint="0" installbase="system32" installname="ddrawex.dll" allowwarnings ="true">  
	<importlibrary definition="ddrawex.def" />
	<include base="ddrawex">.</include>
	<include base="ReactOS">include/reactos/wine-paralles</include>
	<include base="ReactOS">include/reactos/wine</include>

	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="__WINESRC__" />

	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>winspool</library>
	<library>comdlg32</library>
	<library>shell32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>oleaut32</library>
	<library>dxguid</library>
	<library>msvcrt</library>
	<library>advapi32</library>
	<library>ddraw</library>

	<file>main.c</file>
	<file>regsvr.c</file>
      <file>version.rc</file>
</module>
