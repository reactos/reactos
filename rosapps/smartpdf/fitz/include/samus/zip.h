/*
 * ZIP archive
 */

typedef struct sa_zip_s sa_zip;

fz_error *sa_openzip(sa_zip **zipp, char *filename);
void sa_debugzip(sa_zip *zip);
void sa_closezip(sa_zip *zip);
int sa_accesszipentry(sa_zip *zip, char *name);
fz_error *sa_openzipentry(fz_stream **stmp, sa_zip *zip, char *name);

