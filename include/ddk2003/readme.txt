Diffent
need be fixed
Add #define FAR ?

Add ifdef __REACTOS__ in ieverp.h


Look at warning.h see if anything can be done better

Fix rpc.h it is compatible with pseh


add ros hack to upssvc.h so it works again


------------------------------------------------------------
cderr.h
Contain change that does not exists in windows 2003sp1 
But it does exists in ddk for windows 2000/XP, 
it is FNERR_BUFFERTOOSMALL, FRERR_BUFFERLENGTHZERO 

warning.h / rpc.h
Contain one change, we do not use ms seh 
so we need disable the keywords for now 
until gcc support it

