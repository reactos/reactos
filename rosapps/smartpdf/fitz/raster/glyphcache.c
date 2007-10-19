#include "fitz-base.h"
#include "fitz-world.h"
#include "fitz-draw.h"

typedef struct fz_hash_s fz_hash;
typedef struct fz_key_s fz_key;
typedef struct fz_val_s fz_val;

struct fz_glyphcache_s
{
	int slots;
	int size;
	fz_hash *hash;
	fz_val *lru;
	unsigned char *buffer;
	int load;
	int used;
};

struct fz_key_s
{
	void *fid;
	int a, b;
	int c, d;
	unsigned short cid;
	unsigned char e, f;
};

struct fz_hash_s
{
	fz_key key;
	fz_val *val;
};

struct fz_val_s
{
	fz_hash *ent;
	unsigned char *samples;
	short w, h, x, y;
	int uses;
};

static unsigned int hashkey(fz_key *key)
{
	unsigned char *s = (unsigned char*)key;
	unsigned int hash = 0;
	unsigned int i;
	for (i = 0; i < sizeof(fz_key); i++)
	{
		hash += s[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

fz_error *
fz_newglyphcache(fz_glyphcache **arenap, int slots, int size)
{
	fz_glyphcache *arena;

	arena = *arenap = fz_malloc(sizeof(fz_glyphcache));
	if (!arena)
		return fz_outofmem;

	arena->slots = slots;
	arena->size = size;

	arena->hash = nil;
	arena->lru = nil;
	arena->buffer = nil;

	arena->hash = fz_malloc(sizeof(fz_hash) * slots);
	if (!arena->hash)
		goto cleanup;

	arena->lru = fz_malloc(sizeof(fz_val) * slots);
	if (!arena->lru)
		goto cleanup;

	arena->buffer = fz_malloc(size);
	if (!arena->buffer)
		goto cleanup;

	memset(arena->hash, 0, sizeof(fz_hash) * slots);
	memset(arena->lru, 0, sizeof(fz_val) * slots);
	memset(arena->buffer, 0, size);
	arena->load = 0;
	arena->used = 0;

	return nil;

cleanup:
	fz_free(arena->hash);
	fz_free(arena->lru);
	fz_free(arena->buffer);
	fz_free(arena);
	return fz_outofmem;
}

void
fz_dropglyphcache(fz_glyphcache *arena)
{
	fz_free(arena->hash);
	fz_free(arena->lru);
	fz_free(arena->buffer);
	fz_free(arena);
}

static int hokay = 0;
static int hcoll = 0;
static int hdist = 0;
static int coos = 0;
static int covf = 0;

static int ghits = 0;
static int gmisses = 0;

static fz_val *
hashfind(fz_glyphcache *arena, fz_key *key)
{
	fz_hash *tab = arena->hash;
	int pos = hashkey(key) % arena->slots;

	while (1)
	{
		if (!tab[pos].val)
			return nil;

		if (memcmp(key, &tab[pos].key, sizeof (fz_key)) == 0)
			return tab[pos].val;

		pos = pos + 1;
		if (pos == arena->slots)
			pos = 0;
	}
}

static void
hashinsert(fz_glyphcache *arena, fz_key *key, fz_val *val)
{
	fz_hash *tab = arena->hash;
	int pos = hashkey(key) % arena->slots;
int didcoll = 0;

	while (1)
	{
		if (!tab[pos].val)
		{
			tab[pos].key = *key;
			tab[pos].val = val;
			tab[pos].val->ent = &tab[pos];
if (didcoll) hcoll ++; else hokay ++; hdist += didcoll;
			return;
		}

		pos = pos + 1;
		if (pos == arena->slots)
			pos = 0;
didcoll ++;
	}
}

static void
hashremove(fz_glyphcache *arena, fz_key *key)
{
	fz_hash *tab = arena->hash;
	unsigned int pos = hashkey(key) % arena->slots;
	unsigned int hole;
	unsigned int look;
	unsigned int code;

	while (1)
	{
		if (!tab[pos].val)
			return; /* boo hoo! tried to remove non-existant key */

		if (memcmp(key, &tab[pos].key, sizeof (fz_key)) == 0)
		{
			tab[pos].val = nil;

			hole = pos;
			look = hole + 1;
			if (look == arena->slots)
				look = 0;

			while (tab[look].val)
			{
				code = (hashkey(&tab[look].key) % arena->slots);
				if ((code <= hole && hole < look) ||
					(look < code && code <= hole) ||
					(hole < look && look < code))
				{
					tab[hole] = tab[look];
					tab[hole].val->ent = &tab[hole];
					tab[look].val = nil;
					hole = look;
				}

				look = look + 1;
				if (look == arena->slots)
					look = 0;
			}

			return;
		}

		pos = pos + 1;
		if (pos == arena->slots)
			pos = 0;
	}
}

void
fz_debugglyphcache(fz_glyphcache *arena)
{
	printf("cache load %d / %d (%d / %d bytes)\n",
		arena->load, arena->slots, arena->used, arena->size);
	printf("no-colliders: %d colliders: %d\n", hokay, hcoll);
	printf("avg dist: %d / %d: %g\n", hdist, hcoll, (double)hdist / hcoll);
	printf("out-of-space evicts: %d\n", coos);
	printf("out-of-hash evicts: %d\n", covf);
	printf("hits = %d misses = %d ratio = %g\n", ghits, gmisses, (float)ghits / (ghits + gmisses));
/*
	int i;
	for (i = 0; i < arena->slots; i++)
	{
		if (!arena->hash[i].val)
			printf("glyph % 4d: empty\n", i);
		else {
			fz_key *k = &arena->hash[i].key;
			fz_val *b = arena->hash[i].val;
			printf("glyph % 4d: %p %d [%g %g %g %g + %d %d] "
					"-> [%dx%d %d,%d]\n", i,
				k->fid, k->cid,
				k->a / 65536.0,
				k->b / 65536.0,
				k->c / 65536.0,
				k->d / 65536.0,
				k->e, k->f,
				b->w, b->h, b->x, b->y);
		}
	}

	for (i = 0; i < arena->load; i++)
		printf("lru %04d: glyph %d (%d)\n", i,
			arena->lru[i].ent - arena->hash, arena->lru[i].uses);
*/
}

static void
bubble(fz_glyphcache *arena, int i)
{
	fz_val tmp;

	if (i == 0 || arena->load < 2)
		return;

	tmp = arena->lru[i - 1];
	arena->lru[i - 1] = arena->lru[i];
	arena->lru[i] = tmp;

	arena->lru[i - 1].ent->val = &arena->lru[i - 1];
	arena->lru[i].ent->val = &arena->lru[i];
}

static void
evictlast(fz_glyphcache *arena)
{
	fz_val *lru = arena->lru;
	unsigned char *s, *e;
	int i, k;
	fz_key key;

	if (arena->load == 0)
		return;

	k = arena->load - 1;
	s = lru[k].samples;
	e = s + lru[k].w * lru[k].h;

	/* pack buffer to fill hole */
	memmove(s, e, arena->buffer + arena->used - e);
	memset(arena->buffer + arena->used - (e - s), 0, e - s);
	arena->used -= e - s;

	/* update lru pointers */
	for (i = 0; i < k; i++)	/* XXX this is DOG slow! XXX */
		if (lru[i].samples >= e)
			lru[i].samples -= e - s;

	/* remove hash entry */
	key = lru[k].ent->key;
	hashremove(arena, &key);

	arena->load --;
}

static void
evictall(fz_glyphcache *arena)
{
    //printf("zap!\n");
	memset(arena->hash, 0, sizeof(fz_hash) * arena->slots);
	memset(arena->lru, 0, sizeof(fz_val) * arena->slots);
	memset(arena->buffer, 0, arena->size);
	arena->load = 0;
	arena->used = 0;
}

fz_error *
fz_renderglyph(fz_glyphcache *arena, fz_glyph *glyph, fz_font *font, int cid, fz_matrix ctm)
{
	fz_error *error;
	fz_key key;
	fz_val *val;
	int size;

	key.fid = font;
	key.cid = cid;
	key.a = ctm.a * 65536;
	key.b = ctm.b * 65536;
	key.c = ctm.c * 65536;
	key.d = ctm.d * 65536;
	key.e = (ctm.e - fz_floor(ctm.e)) * 256;
	key.f = (ctm.f - fz_floor(ctm.f)) * 256;

	val = hashfind(arena, &key);
	if (val)
	{
		val->uses ++;
		glyph->w = val->w;
		glyph->h = val->h;
		glyph->x = val->x;
		glyph->y = val->y;
		glyph->samples = val->samples;

		bubble(arena, val - arena->lru);

		ghits++;

		return nil;
	}

	gmisses++;

	ctm.e = fz_floor(ctm.e) + key.e / 256.0;
	ctm.f = fz_floor(ctm.f) + key.f / 256.0;

	error = font->render(glyph, font, cid, ctm);
	if (error)
		return error;

	size = glyph->w * glyph->h;

	if (size > arena->size / 6)
		return nil;

	while (arena->load > arena->slots * 75 / 100)
	{
		covf ++;
		evictall(arena);
	}

	while (arena->used + size >= arena->size)
	{
		coos ++;
		evictall(arena);
	}

	val = &arena->lru[arena->load++];
	val->uses = 0;
	val->w = glyph->w;
	val->h = glyph->h;
	val->x = glyph->x;
	val->y = glyph->y;
	val->samples = arena->buffer + arena->used;

	arena->used += size;

	memcpy(val->samples, glyph->samples, glyph->w * glyph->h);
	glyph->samples = val->samples;

	hashinsert(arena, &key, val);

	return nil;
}

