.\msg00001.bin .\userctrs.rc .\userctrs.h : userctrs.mc
        erase .\userctrs.h
        erase .\msg00001.bin
        erase .\userctrs.rc
        mc -v -s userctrs.mc
