/*
 * Metro Package and Parts
 */

typedef struct sa_package_s sa_package;
typedef struct sa_relation_s sa_relation;

struct sa_relation_s
{
	int external;
	char *target;
	char *id;
	char *type;
	sa_relation *next;
};

fz_error *sa_openpackage(sa_package **packp, char *filename);
void sa_debugpackage(sa_package *pack);
void sa_closepackage(sa_package *pack);

fz_error *sa_openpart(fz_stream **stmp, sa_package *pack, char *partname);

char *sa_typepart(sa_package *pack, char *partname);

fz_error *sa_loadrelations(sa_relation **relsp, sa_package *pack, char *partname);
void sa_debugrelations(sa_relation *rels);
void sa_droprelations(sa_relation *rels);

