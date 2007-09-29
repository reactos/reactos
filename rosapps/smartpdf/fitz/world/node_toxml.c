#include "fitz-base.h"
#include "fitz-world.h"

static void indent(int level)
{
	while (level--)
		putchar(' ');
}

static void xmlnode(fz_node *node, int level);

static void xmlmeta(fz_metanode *node, int level)
{
	fz_node *child;

	indent(level);
	printf("<meta name=\"%s\">\n", node->name);

	for (child = node->super.first; child; child = child->next)
		xmlnode(child, level + 1);

	indent(level);
	printf("</meta>\n");
}

static void xmlover(fz_overnode *node, int level)
{
	fz_node *child;
	indent(level);
	printf("<over>\n");
	for (child = node->super.first; child; child = child->next)
		xmlnode(child, level + 1);
	indent(level);
	printf("</over>\n");
}

static void xmlmask(fz_masknode *node, int level)
{
	fz_node *child;
	indent(level);
	printf("<mask>\n");
	for (child = node->super.first; child; child = child->next)
		xmlnode(child, level + 1);
	indent(level);
	printf("</mask>\n");
}

static void xmlblend(fz_blendnode *node, int level)
{
	fz_node *child;
	indent(level);
	printf("<blend mode=\"%d\">\n", node->mode);
	for (child = node->super.first; child; child = child->next)
		xmlnode(child, level + 1);
	indent(level);
	printf("</blend>\n");
}

static void xmltransform(fz_transformnode *node, int level)
{
	indent(level);
	printf("<transform matrix=\"%g %g %g %g %g %g\">\n",
		node->m.a, node->m.b,
		node->m.c, node->m.d,
		node->m.e, node->m.f);
	xmlnode(node->super.first, level + 1);
	indent(level);
	printf("</transform>\n");
}

static void xmlsolid(fz_solidnode *node, int level)
{
	int i;
	indent(level);
	printf("<solid colorspace=\"%s\" v=\"", node->cs->name);
	for (i = 0; i < node->n; i++)
	{
		printf("%g", node->samples[i]);
		if (i < node->n - 1)
			putchar(' ');
	}
	printf("\" />\n");
}

static void xmllink(fz_linknode *node, int level)
{
	indent(level);
	printf("<link name=\"%p\" />\n", node->tree);
}

static void xmlpath(fz_pathnode *node, int level)
{
	int i;

	indent(level);

	if (node->paint == FZ_STROKE)
	{
		printf("<path fill=\"stroke\" cap=\"%d\" join=\"%d\" width=\"%g\" miter=\"%g\"",
			node->linecap,
			node->linejoin,
			node->linewidth,
			node->miterlimit);
		if (node->dash)
		{
			printf(" phase=\"%g\" array=\"", node->dash->phase);
			for (i = 0; i < node->dash->len; i++)
				printf("%g ", node->dash->array[i]);
			printf("\"");
		}
		printf(">\n");
	}
	else
	{
		printf("<path fill=\"%s\">\n",
				node->paint == FZ_FILL ? "nonzero" : "evenodd");
	}

	fz_debugpathnode(node);

	indent(level);
	printf("</path>\n");
}

static void xmltext(fz_textnode *node, int level)
{
	int i;

	indent(level);
	printf("<text font=\"%s\" matrix=\"%g %g %g %g\">\n", node->font->name,
		node->trm.a, node->trm.b, node->trm.c, node->trm.d);

	for (i = 0; i < node->len; i++)
	{
		indent(level + 1);
		if (node->els[i].cid >= 32 && node->els[i].cid < 128)
			printf("<g c=\"%c\" x=\"%g\" y=\"%g\" />\n",
					node->els[i].cid, node->els[i].x, node->els[i].y);
		else
			printf("<g c=\"<%04x>\" x=\"%g\" y=\"%g\" />\n",
					node->els[i].cid, node->els[i].x, node->els[i].y);
	}

	indent(level);
	printf("</text>\n");
}

static void xmlimage(fz_imagenode *node, int level)
{
	fz_image *image = node->image;
	indent(level);
	printf("<image w=\"%d\" h=\"%d\" n=\"%d\" a=\"%d\" />\n",
			image->w, image->h, image->n, image->a);
}

static void xmlshade(fz_shadenode *node, int level)
{
	indent(level);
	printf("<shade />\n");
}

static void xmlnode(fz_node *node, int level)
{
	if (!node)
	{
		indent(level);
		printf("<nil />\n");
		return;
	}

	switch (node->kind)
	{
	case FZ_NMETA: xmlmeta((fz_metanode*)node, level); break;
	case FZ_NOVER: xmlover((fz_overnode*)node, level); break;
	case FZ_NMASK: xmlmask((fz_masknode*)node, level); break;
	case FZ_NBLEND: xmlblend((fz_blendnode*)node, level); break;
	case FZ_NTRANSFORM: xmltransform((fz_transformnode*)node, level); break;
	case FZ_NCOLOR: xmlsolid((fz_solidnode*)node, level); break;
	case FZ_NPATH: xmlpath((fz_pathnode*)node, level); break;
	case FZ_NTEXT: xmltext((fz_textnode*)node, level); break;
	case FZ_NIMAGE: xmlimage((fz_imagenode*)node, level); break;
	case FZ_NSHADE: xmlshade((fz_shadenode*)node, level); break;
	case FZ_NLINK: xmllink((fz_linknode*)node, level); break;
	}
}

void
fz_debugnode(fz_node *node)
{
	xmlnode(node, 0);
}

void
fz_debugtree(fz_tree *tree)
{
	printf("<tree>\n");
	xmlnode(tree->root, 1);
	printf("</tree>\n");
}

