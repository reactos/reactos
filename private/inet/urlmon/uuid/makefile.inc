
#copy the produced file to the chicago libs, to make cairole for chicago happy

LIBDIR=$(BASEDIR)\public\sdk\lib
LIBFILE=$(TARGET_DIRECTORY)\uuidmon3.lib


$(LIBDIR)\chicago\$(LIBFILE) : $(LIBDIR)\$(LIBFILE)
    copy $(LIBDIR)\$(LIBFILE) $(LIBDIR)\chicago\$(LIBFILE)




