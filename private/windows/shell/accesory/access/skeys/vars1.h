/*  VARS1.H  */


#define MAXALIASLEN 20
#define CODEBUFFERLEN 20
#define MAXVECTORSTACK 20
#define MAXLISTLENGTH 20

struct aliasTable {
	unsigned char *aliasName;
	unsigned char gideiCode;
	};

struct asciiTables {
	unsigned char gideiCode1, gideiCode2;
	};

struct listTypes {
	int len;
	unsigned char list[MAXLISTLENGTH];
	};



