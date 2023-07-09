obj\$(TARGET_DIRECTORY)\res_str.i: res_str.h
    $(TARGET_CPP) -EP -DRESOURCES $(CDEFINES) $(INCPATH0) -Tc res_str.h > $@


obj\$(TARGET_DIRECTORY)\em.res: obj\$(TARGET_DIRECTORY)\res_str.i

