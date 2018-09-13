/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
//This file emulates the standard TOOLS.H

#define MAXLINELEN  1024        /* longest line of input */
#define TRUE            1
#define FALSE       0

typedef char flagType;
typedef char * (*TOOLS_ALLOC) (unsigned int);
extern TOOLS_ALLOC tools_alloc;
void Move(void FAR *, void FAR *, unsigned int);
void Fill(void FAR *, char, unsigned int);
char *strbskip(char const *, char const *);
extern char XLTab[], XUTab[];
