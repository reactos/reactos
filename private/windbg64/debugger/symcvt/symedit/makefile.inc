$(O)\strings.i: strings.h
    $(TARGET_CPP) -EP -DRESOURCES -Tc strings.h > $@

$(O)\symedit.res: $(O)\strings.i
