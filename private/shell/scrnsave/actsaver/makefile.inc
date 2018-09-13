# rules to generate the tlb and h file

clean:
    del /F /Q $(NTTARGETFILE0)
    del /F /Q $(NTTARGETFILES)

actsaver.tlb actsaver_i.c $(IFACE_DIR)\actsaver.h: actsaver.idl
