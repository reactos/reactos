all: advapi32.a

TOKEN_OBJECTS = token/access.o 

REGISTRY_OBJECTS = reg/reg.o

SEC_OBJECTS = sec/sec.o sec/rtlsec.o



OBJECTS = $(TOKEN_OBJECTS) $(REGISTRY_OBJECTS) $(SEC_OBJECTS) 
  

advapi32.a: $(OBJECTS)
	$(AR) vrcs advapi32.a $(OBJECTS) 

dummy:

include ../../rules.mak
