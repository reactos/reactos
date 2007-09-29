#include "fitz-base.h"
#include "fitz-world.h"

/*
 * Remove (mask ... white) until we get something not white
 */

static int iswhitenode(fz_solidnode *node)
{
	if (!strcmp(node->cs->name, "DeviceGray"))
		return fabs(node->samples[0] - 1.0) < FLT_EPSILON;
	if (!strcmp(node->cs->name, "DeviceRGB"))
		return fabs(node->samples[0] - 1.0) < FLT_EPSILON &&
				fabs(node->samples[1] - 1.0) < FLT_EPSILON &&
				fabs(node->samples[2] - 1.0) < FLT_EPSILON;
	if (!strcmp(node->cs->name, "DeviceCMYK"))
		return fabs(node->samples[0]) < FLT_EPSILON &&
				fabs(node->samples[1]) < FLT_EPSILON &&
				fabs(node->samples[2]) < FLT_EPSILON &&
				fabs(node->samples[3]) < FLT_EPSILON;
	return 0;
}

static int cleanwhite(fz_node *node)
{
	fz_node *current;
	fz_node *next;
	fz_node *shape;
	fz_node *color;

	for (current = node->first; current; current = next)
	{
		next = current->next;

		if (fz_islinknode(current))
			return 1;
		else if (fz_isimagenode(current))
			return 1;
		else if (fz_isshadenode(current))
			return 1;
		else if (fz_issolidnode(current))
		{
			if (!iswhitenode((fz_solidnode*)current))
				return 1;
		}

		else if (fz_ismasknode(current))
		{
			shape = current->first;
			color = shape->next;
			if (fz_issolidnode(color))
			{
				if (iswhitenode((fz_solidnode*)color))
					fz_removenode(current);
				else
					return 1;
			}
			else
			{
				if (cleanwhite(current))
					return 1;
			}
		}

		else
		{
			if (cleanwhite(current))
				return 1;
		}
	}

	return 0;
}

/*
 * Remove useless overs that only have one child.
 */

static void cleanovers(fz_node *node)
{
	fz_node *prev;
	fz_node *next;
	fz_node *current;
	fz_node *child;

	prev = nil;
	for (current = node->first; current; current = next)
	{
		next = current->next;

		if (fz_isovernode(current))
		{
			if (current->first == current->last)
			{
				child = current->first;
				fz_removenode(current);
				if (child)
				{
					if (prev)
						fz_insertnodeafter(prev, child);
					else
						fz_insertnodefirst(node, child);
				}
				current = child;
			}
		}

		if (current)
			prev = current;
	}

	for (current = node->first; current; current = current->next)
		cleanovers(current);
}

/*
 * Remove rectangular clip-masks whose contents fit...
 */

static int getrect(fz_pathnode *path, fz_rect *bboxp)
{
	float x, y, w, h;

	/* move x y, line x+w y, line x+w y+h, line x y+h, close */

	if (path->len != 13)
		return 0;

	if (path->els[0].k != FZ_MOVETO) return 0;
	x = path->els[1].v;
	y = path->els[2].v;

	if (path->els[3].k != FZ_LINETO) return 0;
	w = path->els[4].v - x;
	if (path->els[5].v != y) return 0;

	if (path->els[6].k != FZ_LINETO) return 0;
	if (path->els[7].v != x + w) return 0;
	h = path->els[8].v - y;

	if (path->els[9].k != FZ_LINETO) return 0;
	if (path->els[10].v != x) return 0;
	if (path->els[11].v != y + h) return 0;

	if (path->els[12].k != FZ_CLOSEPATH) return 0;

	bboxp->x0 = MIN(x, x + w);
	bboxp->y0 = MIN(y, y + h);
	bboxp->x1 = MAX(x, x + w);
	bboxp->y1 = MAX(y, y + h);

	return 1;
}

static int fitsinside(fz_node *node, fz_rect clip)
{
	fz_rect bbox;
	bbox = fz_boundnode(node, fz_identity());
	if (fz_isinfiniterect(bbox)) return 0;
	if (fz_isemptyrect(bbox)) return 1;
	if (bbox.x0 < clip.x0) return 0;
	if (bbox.x1 > clip.x1) return 0;
	if (bbox.y0 < clip.y0) return 0;
	if (bbox.y1 > clip.y1) return 0;
	return 1;
}

static void cleanmasks(fz_node *node)
{
	fz_node *prev;
	fz_node *current;
	fz_node *shape;
	fz_node *color;
	fz_rect bbox;

	for (current = node->first; current; current = current->next)
		cleanmasks(current);

	prev = nil;
	for (current = node->first; current; current = current->next)
	{
retry:
		if (!current)
			break;

		if (fz_ismasknode(current))
		{
			shape = current->first;
			color = shape->next;

			if (color == nil)
			{
				fz_removenode(current);
				prev = nil;
				current = node->first;
				goto retry;
			}

			if (fz_ispathnode(shape))
			{
				if (getrect((fz_pathnode*)shape, &bbox))
				{
					if (fitsinside(color, bbox))
					{
						fz_removenode(current);
						if (prev)
							fz_insertnodeafter(prev, color);
						else
							fz_insertnodefirst(node, color);
						current = color;
						goto retry;
					}
				}
			}
		}

		prev = current;
	}
}

/*
 * Turn 1x1 images into rectangle fills
 */

static fz_error *clean1x1(fz_node *node)
{
	fz_error *error;
	fz_node *current;
	fz_node *color;
	fz_pathnode *rect;
	fz_node *mask;
	fz_image *image;
	fz_pixmap *pix;
	float v[FZ_MAXCOLORS];
	int i;

	for (current = node->first; current; current = current->next)
	{
		if (fz_isimagenode(current))
		{
			image = ((fz_imagenode*)current)->image;
			if (image->w == 1 && image->h == 1)
			{
				error = fz_newpathnode(&rect);
				fz_moveto(rect, 0, 0);
				fz_lineto(rect, 1, 0);
				fz_lineto(rect, 1, 1);
				fz_lineto(rect, 0, 1);
				fz_closepath(rect);
				fz_endpath(rect, FZ_FILL, nil, nil);

				if (image->cs)
				{
					error = fz_newpixmap(&pix, 0, 0, 1, 1, image->n + 1);
					if (error)
						return error;

					error = image->loadtile(image, pix);
					if (error)
						return error;

					for (i = 0; i < image->n; i++)
						v[i] = pix->samples[i + 1] / 255.0;

					fz_droppixmap(pix);

					error = fz_newsolidnode(&color, image->cs, image->n, v);
					if (error)
						return error;
					error = fz_newmasknode(&mask);
					if (error)
						return error;

					fz_insertnodeafter(current, mask);
					fz_insertnodelast(mask, (fz_node*)rect);
					fz_insertnodelast(mask, color);
					fz_removenode(current);
					current = mask;
				}

				else
				{
					/* pray that the 1x1 image mask is all opaque */
					fz_insertnodeafter(current, (fz_node*)rect);
					fz_removenode(current);
					current = (fz_node*)rect;
				}
			}
		}

		error = clean1x1(current);
		if (error)
			return error;
	}

	return nil;
}

/*
 *
 */

fz_error *
fz_optimizetree(fz_tree *tree)
{
	if (getenv("DONTOPT"))
		return nil;
	cleanwhite(tree->root);
	cleanovers(tree->root);
	cleanmasks(tree->root);
	clean1x1(tree->root);
	return nil;
}

