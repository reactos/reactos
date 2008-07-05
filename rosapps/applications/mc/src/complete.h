#ifndef __COMPLETE_H
#define __COMPLETE_H

#define INPUT_COMPLETE_FILENAMES	 1
#define INPUT_COMPLETE_HOSTNAMES	 2
#define INPUT_COMPLETE_COMMANDS		 4
#define INPUT_COMPLETE_VARIABLES	 8
#define INPUT_COMPLETE_USERNAMES	16
#define INPUT_COMPLETE_CD		32

typedef char *CompletionFunction (char *, int);

void free_completions (WInput *);
void complete (WInput *);

#endif	/* __COMPLETE_H */
