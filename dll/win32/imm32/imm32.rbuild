<module name="imm32" type="win32dll" baseaddress="${BASEADDRESS_IMM32}" installbase="system32" installname="imm32.dll" allowwarnings="true">
	<importlibrary definition="imm32.spec.def" />
	<include base="imm32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>imm.c</file>
	<file>version.rc</file>
	<file>imm32.spec</file>
</module>
