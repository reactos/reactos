This is a collection of simple tests apps I have ported to the
ros build system from wine and other places. Most if not all 
work great under Win9x,NT,2k and XP. I've fixed and renamed a few 
of these because the old names didn't really describe them.

If you feel like messing with this just type make_install and 
everything will be installed to C:\reactos\bin\tests

TESTS -
GetSystemInfo: Reads from kernel32.dll

guitest: simple win32 gui test

hello: another win32 hello window test

hello2: yet another win32 hello window test

Mutex: Mutex benchmarks from the wineserver kernel module

new: example of create new window

Parent_Child: example of child windows inside of parents

rolex: a clock program worth $30,000

volinfo - This gets the volume info for all local and network drives
AVOID THIS ON 9X, it works but its very slow.




