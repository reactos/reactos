wsock32.rc: nlstxt.rc msg00001.bin

nlstxt.h nlstxt.rc msg00001.bin nlstxt.mc: ..\..\..\libuemul\libuemul.mc ..\..\..\sockutil\sockutil.mc localmsg.mc
    copy /a ..\..\..\libuemul\libuemul.mc+..\..\..\sockutil\sockutil.mc+localmsg.mc nlstxt.mc
    mc -v nlstxt.mc

clean::
    del  nlstxt.h nlstxt.rc msg00001.bin nlstxt.mc

