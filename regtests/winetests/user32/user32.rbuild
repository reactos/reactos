<module name="user32_winetest" type="win32cui" installbase="bin" installname="user32_winetest.exe" allowwarnings="true">
    <include base="user32_winetest">.</include>
    <define name="__USE_W32API" />
    <library>ntdll</library>
    <library>user32</library>
    <library>gdi32</library>
    <library>advapi32</library>
    <library>kernel32</library>
    <file>class.c</file>
    <file>clipboard.c</file>
    <file>dce.c</file>
    <file>dde.c</file>
    <file>dialog.c</file>
    <file>edit.c</file>
    <file>input.c</file>
    <file>listbox.c</file>
    <file>menu.c</file>
    <file>monitor.c</file>
    <file>msg.c</file>
    <file>resource.c</file>
    <file>sysparams.c</file>
    <file>text.c</file>
    <file>win.c</file>
    <file>winstation.c</file>
    <file>wsprintf.c</file>
    <file>testlist.c</file>
    <file>resource.rc</file>
</module>
