<module name="gdiplus" type="win32dll" baseaddress="${BASEADDRESS_GDIPLUS}" installbase="system32" installname="gdiplus.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="gdiplus.spec.def" />
        <include base="gdiplus">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <define name="__USE_W32API" />
        <define name="_WIN32_IE">0x600</define>
        <define name="_WIN32_WINNT">0x501</define>
        <define name="WINVER">0x501</define>
        <library>wine</library>
        <library>shlwapi</library>
        <library>oleaut32</library>
        <library>ole32</library>
        <library>user32</library>
        <library>gdi32</library>
        <library>kernel32</library>
        <library>uuid</library>
        <library>ntdll</library>
        <file>brush.c</file>
        <file>customlinecap.c</file>
        <file>font.c</file>
        <file>gdiplus.c</file>
        <file>graphics.c</file>
        <file>graphicspath.c</file>
        <file>image.c</file>
        <file>imageattributes.c</file>
        <file>matrix.c</file>
        <file>pathiterator.c</file>
        <file>pen.c</file>
        <file>stringformat.c</file>
		<file>region.c</file>
        <file>gdiplus.spec</file>
</module>