To generate a new scanner and parser run:

flex --header-file=parser.yy.h --outfile=parser.yy.c parser.l

bison --defines=parser.tab.h --name-prefix=parser_ parser.y
