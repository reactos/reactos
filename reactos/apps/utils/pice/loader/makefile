CC = gcc
CFLAGS := -Wall -fomit-frame-pointer -O2  -DLINUX
LDFLAGS := -dynamic -lncurses

OBJS = main.o terminal.o 


loader: 	$(OBJS)
		$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

clean:		
		@echo deleting all intermediate files
		@rm $(OBJS) loader -f
		make