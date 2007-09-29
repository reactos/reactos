/*
 * The display tree is at the center of attention in Fitz.
 * The tree and most of its minor nodes.
 * Paths and text nodes are found elsewhere.
 */

typedef struct fz_tree_s fz_tree;
typedef struct fz_node_s fz_node;

struct fz_tree_s
{
	int refs;
	fz_node *root;
	fz_node *head;
};

/* tree operations */
fz_error *fz_newtree(fz_tree **treep);
fz_tree *fz_keeptree(fz_tree *tree);
void fz_droptree(fz_tree *tree);

fz_rect fz_boundtree(fz_tree *tree, fz_matrix ctm);
void fz_debugtree(fz_tree *tree);
void fz_insertnodefirst(fz_node *parent, fz_node *child);
void fz_insertnodelast(fz_node *parent, fz_node *child);
void fz_insertnodeafter(fz_node *prev, fz_node *child);
void fz_removenode(fz_node *child);

fz_error *fz_optimizetree(fz_tree *tree);

/* node types */

typedef struct fz_transformnode_s fz_transformnode;
typedef struct fz_overnode_s fz_overnode;
typedef struct fz_masknode_s fz_masknode;
typedef struct fz_blendnode_s fz_blendnode;
typedef struct fz_pathnode_s fz_pathnode;
typedef struct fz_textnode_s fz_textnode;
typedef struct fz_solidnode_s fz_solidnode;
typedef struct fz_imagenode_s fz_imagenode;
typedef struct fz_shadenode_s fz_shadenode;
typedef struct fz_linknode_s fz_linknode;
typedef struct fz_metanode_s fz_metanode;

typedef enum fz_nodekind_e
{
	FZ_NTRANSFORM,
	FZ_NOVER,
	FZ_NMASK,
	FZ_NBLEND,
	FZ_NPATH,
	FZ_NTEXT,
	FZ_NCOLOR,
	FZ_NIMAGE,
	FZ_NSHADE,
	FZ_NLINK,
	FZ_NMETA
} fz_nodekind;

typedef enum fz_blendkind_e
{
	/* PDF 1.4 -- standard separable */
	FZ_BNORMAL,
	FZ_BMULTIPLY,
	FZ_BSCREEN,
	FZ_BOVERLAY,
	FZ_BDARKEN,
	FZ_BLIGHTEN,
	FZ_BCOLORDODGE,
	FZ_BCOLORBURN,
	FZ_BHARDLIGHT,
	FZ_BSOFTLIGHT,
	FZ_BDIFFERENCE,
	FZ_BEXCLUSION,

	/* PDF 1.4 -- standard non-separable */
	FZ_BHUE,
	FZ_BSATURATION,
	FZ_BCOLOR,
	FZ_BLUMINOSITY,

	FZ_BOVERPRINT,
	FZ_BRASTEROP
} fz_blendkind;

struct fz_node_s
{
	fz_nodekind kind;
	fz_node *parent;
	fz_node *first;
	fz_node *last;
	fz_node *next;
};

struct fz_transformnode_s
{
	fz_node super;
	fz_matrix m;
};

struct fz_overnode_s
{
	fz_node super;
};

struct fz_masknode_s
{
	fz_node super;
};

struct fz_blendnode_s
{
	fz_node super;
	fz_colorspace *cs;
	fz_blendkind mode;
	int isolated;
	int knockout;
};

struct fz_solidnode_s
{
	fz_node super;
	fz_colorspace *cs;
	int n;
	float samples[FZ_FLEX];
};

struct fz_linknode_s
{
	fz_node super;
	fz_tree *tree;
};

struct fz_metanode_s
{
	fz_node super;
	char *name;
	void *dict;
};

struct fz_imagenode_s
{
	fz_node super;
	fz_image *image;
};

struct fz_shadenode_s
{
	fz_node super;
	fz_shade *shade;
};

/* common to all nodes */
void fz_initnode(fz_node *node, fz_nodekind kind);
fz_rect fz_boundnode(fz_node *node, fz_matrix ctm);
void fz_dropnode(fz_node *node);

/* branch nodes */
fz_error *fz_newmetanode(fz_node **nodep, char *name, void *dict);
fz_error *fz_newovernode(fz_node **nodep);
fz_error *fz_newmasknode(fz_node **nodep);
fz_error *fz_newblendnode(fz_node **nodep, fz_colorspace *cs, fz_blendkind b, int k, int i);
fz_error *fz_newtransformnode(fz_node **nodep, fz_matrix m);

int fz_istransformnode(fz_node *node);
int fz_isovernode(fz_node *node);
int fz_ismasknode(fz_node *node);
int fz_isblendnode(fz_node *node);
int fz_ismetanode(fz_node *node);

/* leaf nodes */
fz_error *fz_newlinknode(fz_node **nodep, fz_tree *subtree);
fz_error *fz_newsolidnode(fz_node **nodep, fz_colorspace *cs, int n, float *v);
fz_error *fz_newimagenode(fz_node **nodep, fz_image *image);
fz_error *fz_newshadenode(fz_node **nodep, fz_shade *shade);

int fz_islinknode(fz_node *node);
int fz_issolidnode(fz_node *node);
int fz_ispathnode(fz_node *node);
int fz_istextnode(fz_node *node);
int fz_isimagenode(fz_node *node);
int fz_isshadenode(fz_node *node);

