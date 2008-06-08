<module name="gdiplus_winetest" type="win32cui" installbase="bin" installname="gdiplus_winetest.exe" allowwarnings="true">
        <include base="gdiplus_winetest">.</include>
        <define name="__USE_W32API" />
        <define name="_WIN32_IE">0x600</define>
        <define name="_WIN32_WINNT">0x501</define>
        <define name="WINVER">0x501</define>
        <library>wine</library>
        <library>gdiplus</library>
        <library>user32</library>
        <library>gdi32</library>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>brush.c</file>
        <file>font.c</file>
        <file>graphics.c</file>
        <file>graphicspath.c</file>
        <file>image.c</file>
        <file>matrix.c</file>
        <file>pen.c</file>
        <file>stringformat.c</file>
        <file>testlist.c</file>
</module>