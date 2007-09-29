#include "fitz-base.h"
#include "fitz-world.h"

static void indent(int level)
{
	while (level--)
		putchar(' ');
}

static void lispnode(fz_node *node, int level);

static void lispmeta(fz_metanode *node, int level)
{
	fz_node *child;
	indent(level);
	printf("(meta ");
	if (node->name) { fz_debugobj(node->name); }
	if (node->dict) { printf("\n"); fz_debugobj(node->dict); }
	printf("\n");
	for (child = node->super.first; child; child = child->next)
		lispnode(child, level + 1);
	indent(level);
	printf(")\n");
}

static void lispover(fz_overnode *node, int level)
{
	fz_node *child;
	indent(level);
	printf("(over\n");
	for (child = node->super.first; child; child = child->next)
		lispnode(child, level + 1);
	indent(level);
	printf(")\n");
}

static void lispmask(fz_masknode *node, int level)
{
	fz_node *child;
	indent(level);
	printf("(mask\n");
	for (child = node->super.first; child; child = child->next)
		lispnode(child, level + 1);
	indent(level);
	printf(")\n");
}

static void lispblend(fz_blendnode *node, int level)
{
	fz_node *child;
	indent(level);
	printf("(blend-%d\n", node->mode);
	for (child = node->super.first; child; child = child->next)
		lispnode(child, level + 1);
	indent(level);
	printf(")\n");
}

static void lisptransform(fz_transformnode *node, int level)
{
	indent(level);
	printf("(transform %g %g %g %g %g %g\n",
		node->m.a, node->m.b,
		node->m.c, node->m.d,
		node->m.e, node->m.f);
	lispnode(node->super.first, level + 1);
	indent(level);
	printf(")\n");
}

static void lispcolor(fz_colornode *node, int level)
{
	int i;
	indent(level);
	printf("(color %s ", node->cs->name);
	for (i = 0; i < node->n; i++)
		printf("%g ", node->samples[i]);
	printf(")\n");
}

static void lisplink(fz_linknode *node, int level)
{
	indent(level);
	printf("(link %p)\n", node->tree);
}

static void lisppath(fz_pathnode *node, int level)
{
	int i;

	indent(level);

	if (node->paint == FZ_STROKE)
	{
		printf("(path 'stroke %d %d %g %g ",
			node->linecap,
			node->linejoin,
			node->linewidth,
			node->miterlimit);
		if (node->dash)
		{
			printf("%g '( ", node->dash->phase);
			for (i = 0; i < node->dash->len; i++)
				printf("%g ", node->dash->array[i]);
			printf(")");
		}
		else
			printf("0 '()");
	}
	else
	{
		printf("(path '%s", node->paint == FZ_FILL ? "fill" : "eofill");
	}

	printf("\n");
	fz_debugpathnode(node);

	indent(level);
	printf(")\n");
}

static void lisptext(fz_textnode *node, int level)
{
	int i;

	indent(level);
	printf("(text %s [%g %g %g %g]\n", node->font->name,
		node->trm.a, node->trm.b, node->trm.c, node->trm.d);

	for (i = 0; i < node->len; i++)
	{
		indent(level + 1);
		if (node->els[i].cid >= 32 && node->els[i].cid < 128)
			printf("(cid '%c' %g %g)\n", node->els[i].cid, node->els[i].x, node->els[i].y);
		else
			printf("(cid <%04x> %g %g)\n", node->els[i].cid, node->els[i].x, node->els[i].y);
	}

	indent(level);
	printf(")\n");
}

static void lispimage(fz_imagenode *node, int level)
{
	fz_image *image = node->image;
	indent(level);
	printf("(image %dx%d %d+%d)\n", image->w, image->h, image->n, image->a);
}

static void lispshade(fz_shadenode *node, int level)
{
	indent(level);
	printf("(shade)\n");
}

static void lispnode(fz_node *node, int level)
{
	if (!node)
	{
		indent(level);
		printf("(nil)\n");
		return;
	}

	switch (node->kind)
	{
	case FZ_NMETA: lispmeta((fz_metanode*)node, level); break;
	case FZ_NOVER: lispover((fz_overnode*)node, level); break;
	case FZ_NMASK: lispmask((fz_masknode*)node, level); break;
	case FZ_NBLEND: lispblend((fz_blendnode*)node, level); break;
	case FZ_NTRANSFORM: lisptransform((fz_transformnode*)node, level); break;
	case FZ_NCOLOR: lispcolor((fz_colornode*)node, level); break;
	case FZ_NPATH: lisppath((fz_pathnode*)node, level); break;
	case FZ_NTEXT: lisptext((fz_textnode*)node, level); break;
	case FZ_NIMAGE: lispimage((fz_imagenode*)node, level); break;
	case FZ_NSHADE: lispshade((fz_shadenode*)node, level); break;
	case FZ_NLINK: lisplink((fz_linknode*)node, level); break;
	}
}

void
fz_debugnode(fz_node *node)
{
	lispnode(node, 0);
}

void
fz_debugtree(fz_tree *tree)
{
	lispnode(tree->root, 0);
}

