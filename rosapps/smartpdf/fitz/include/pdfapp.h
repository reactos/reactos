/*
 * Utility object for handling a pdf application / view
 * Takes care of PDF loading and displaying and navigation,
 * uses a number of callbacks to the GUI app.
 */

typedef struct pdfapp_s pdfapp_t;

enum { ARROW, HAND, WAIT };

extern void winwarn(pdfapp_t*, char *s);
extern void winerror(pdfapp_t*, char *s);
extern void wintitle(pdfapp_t*, char *title);
extern void winresize(pdfapp_t*, int w, int h);
extern void winconvert(pdfapp_t*, fz_pixmap *image);
extern void winrepaint(pdfapp_t*);
extern char* winpassword(pdfapp_t*, char *filename);
extern void winopenuri(pdfapp_t*, char *s);
extern void wincursor(pdfapp_t*, int curs);
extern void windocopy(pdfapp_t*);

struct pdfapp_s
{
	/* current document params */
	char *filename;
	char *doctitle;
	pdf_xref *xref;
	pdf_outline *outline;
	pdf_pagetree *pages;
	fz_renderer *rast;

	/* current view params */
	float zoom;
	int rotate;
	fz_pixmap *image;

	/* current page params */
	int pageno;
	pdf_page *page;

	/* snapback history */
	int hist[256];
	int histlen;

	/* window system sizes */
	int winw, winh;
	int scrw, scrh;
	int shrinkwrap;

	/* event handling state */
	char number[256];
	int numberlen;

	int ispanning;
	int panx, pany;

	int iscopying;
	int selx, sely;
	fz_irect selr;
        
        /* client context storage */
        void *userdata;
};

void pdfapp_init(pdfapp_t *app);
void pdfapp_open(pdfapp_t *app, char *filename);
void pdfapp_close(pdfapp_t *app);

char *pdfapp_usage(pdfapp_t *app);

void pdfapp_onkey(pdfapp_t *app, int c);
void pdfapp_onmouse(pdfapp_t *app, int x, int y, int btn, int modifiers, int state);
void pdfapp_oncopy(pdfapp_t *app, unsigned short *ucsbuf, int ucslen);
void pdfapp_onresize(pdfapp_t *app, int w, int h);

