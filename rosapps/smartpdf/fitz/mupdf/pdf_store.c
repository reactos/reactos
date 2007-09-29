#include <fitz.h>
#include <mupdf.h>

typedef struct pdf_item_s pdf_item;

struct pdf_item_s
{
	pdf_itemkind kind;
	fz_obj *key;
	void *val;
	pdf_item *next;
};

struct refkey
{
	pdf_itemkind kind;
	int oid;
	int gen;
};

struct pdf_store_s
{
	fz_hashtable *hash;		/* hash for oid/gen keys */
	pdf_item *root;		/* linked list for everything else */
};

fz_error *
pdf_newstore(pdf_store **storep)
{
	fz_error *error;
	pdf_store *store;

	store = fz_malloc(sizeof(pdf_store));
	if (!store)
		return fz_outofmem;

	error = fz_newhash(&store->hash, 4096, sizeof(struct refkey));
	if (error)
	{
		fz_free(store);
		return error;
	}

	store->root = nil;

	*storep = store;
	return nil;
}

static void dropitem(pdf_itemkind kind, void *val)
{
    switch (kind)
    {
    case PDF_KCOLORSPACE: fz_dropcolorspace(val); break;
    case PDF_KFUNCTION: pdf_dropfunction(val); break;
    case PDF_KXOBJECT: pdf_dropxobject(val); break;
    case PDF_KIMAGE: fz_dropimage(val); break;
    case PDF_KPATTERN: pdf_droppattern(val); break;
    case PDF_KSHADE: fz_dropshade(val); break;
    case PDF_KCMAP: pdf_dropcmap(val); break;
    case PDF_KFONT: fz_dropfont(val); break;
    }
}

void
pdf_emptystore(pdf_store *store)
{
	pdf_item *item;
	pdf_item *next;
	struct refkey *key;
	void *val;
	int i;

	for (i = 0; i < fz_hashlen(store->hash); i++)
	{
		key = fz_hashgetkey(store->hash, i);
		val = fz_hashgetval(store->hash, i);
		if (val)
			dropitem(key->kind, val);
	}
	fz_emptyhash(store->hash);

	for (item = store->root; item; item = next)
	{
		next = item->next;
		fz_dropobj(item->key);
		dropitem(item->kind, item->val);
		fz_free(item);
	}

	store->root = nil;
}

void
pdf_dropstore(pdf_store *store)
{
	pdf_emptystore(store);
	fz_drophash(store->hash);
	fz_free(store);
}

fz_error *
pdf_storeitem(pdf_store *store, pdf_itemkind kind, fz_obj *key, void *val)
{
    fz_error *error;

    switch (kind)
    {
    case PDF_KCOLORSPACE: fz_keepcolorspace(val); break;
    case PDF_KFUNCTION: pdf_keepfunction(val); break;
    case PDF_KXOBJECT: pdf_keepxobject(val); break;
    case PDF_KIMAGE: fz_keepimage(val); break;
    case PDF_KPATTERN: 
        pdf_keeppattern(val); 
        break;
    case PDF_KSHADE: fz_keepshade(val); break;
    case PDF_KCMAP: pdf_keepcmap(val); break;
    case PDF_KFONT: fz_keepfont(val); break;
    }

    if (fz_isindirect(key))
    {
        struct refkey item;

        pdf_logrsrc("store item %d: %d %d R = %p\n", kind, fz_tonum(key), fz_togen(key), val);

        item.kind = kind;
        item.oid = fz_tonum(key);
        item.gen = fz_togen(key);

        error = fz_hashinsert(store->hash, &item, val);
        if (error)
            return error;
    }

    else
    {
        pdf_item *item;

        item = fz_malloc(sizeof(pdf_item));
        if (!item)
            return fz_outofmem;

        pdf_logrsrc("store item %d: ... = %p\n", kind, val);

        item->kind = kind;
        item->key = fz_keepobj(key);
        item->val = val;

        item->next = store->root;
        store->root = item;
    }

    return nil;
}

void *
pdf_finditem(pdf_store *store, pdf_itemkind kind, fz_obj *key)
{
	pdf_item *item;
	struct refkey refkey;

	if (key == nil)
		return nil;

	if (fz_isindirect(key))
	{
		refkey.kind = kind;
		refkey.oid = fz_tonum(key);
		refkey.gen = fz_togen(key);
		return fz_hashfind(store->hash, &refkey);
	}

	else
	{
		for (item = store->root; item; item = item->next)
			if (item->kind == kind && !fz_objcmp(item->key, key))
				return item->val;
	}

	return nil;
}

