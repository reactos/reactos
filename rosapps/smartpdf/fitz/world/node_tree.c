#include "fitz-base.h"
#include "fitz-world.h"

fz_error *
fz_newtree(fz_tree **treep)
{
    fz_tree *tree;

    tree = *treep = fz_malloc(sizeof (fz_tree));
    if (!tree)
        return fz_outofmem;

    tree->refs = 1;
    tree->root = nil;
    tree->head = nil;

    return nil;
}

fz_tree *
fz_keeptree(fz_tree *tree)
{
    assert(tree->refs > 0);
    tree->refs ++;
    return tree;
}

void
fz_droptree(fz_tree *tree)
{
    assert(tree->refs > 0);
    if (--tree->refs == 0)
    {
        if (tree->root)
            fz_dropnode(tree->root);
        fz_free(tree);
    }
}

fz_rect
fz_boundtree(fz_tree *tree, fz_matrix ctm)
{
    if (tree->root)
        return fz_boundnode(tree->root, ctm);
    return fz_emptyrect;
}

void
fz_insertnodefirst(fz_node *parent, fz_node *child)
{
    child->parent = parent;
    child->next = parent->first;
    parent->first = child;
    if (!parent->last)
        parent->last = child;
}

void
fz_insertnodelast(fz_node *parent, fz_node *child)
{
    child->parent = parent;
    if (!parent->first)
        parent->first = child;
    else
        parent->last->next = child;
    parent->last = child;
}

void
fz_insertnodeafter(fz_node *prev, fz_node *child)
{
	fz_node *parent = prev->parent;
	child->parent = parent;
	if (parent->last == prev)
		parent->last = child;
	child->next = prev->next;
	prev->next = child;
}

void
fz_removenode(fz_node *child)
{
	fz_node *parent = child->parent;
	fz_node *prev;
	fz_node *node;

	if (parent->first == child)
	{
		parent->first = child->next;
		if (parent->last == child)
			parent->last = nil;
		return;
	}

	prev = parent->first;
	node = prev->next;

	while (node)
	{
		if (node == child)
		{
			prev->next = child->next;
		}
		prev = node;
		node = node->next;
	}

	parent->last = prev;
}

