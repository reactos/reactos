#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <elf.h>
#include "stab_gnu.h"
#include "retypes.h"
#include "terminal.h"
#include <termios.h>
#include "../shared/shared.h"


