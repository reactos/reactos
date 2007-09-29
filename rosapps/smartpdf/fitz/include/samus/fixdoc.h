/*
 * FixedDocumentSequence -- the list of pages
 */

typedef struct sa_fixdocseq_s sa_fixdocseq;

fz_error *sa_loadfixdocseq(sa_fixdocseq **seqp, sa_package *pack, char *part);
void sa_debugfixdocseq(sa_fixdocseq *seq);
void sa_dropfixdocseq(sa_fixdocseq *seq);

int sa_getpagecount(sa_fixdocseq *seq);
char *sa_getpagepart(sa_fixdocseq *seq, int idx);

