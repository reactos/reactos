/*
 * Metro physical packages and parts, mapped to a zip archive.
 */

#include "fitz.h"
#include "samus.h"

typedef struct sa_default_s sa_default;
typedef struct sa_override_s sa_override;

struct sa_package_s
{
	sa_zip *zip;
	sa_default *defaults;
	sa_override *overrides;
};

/*
 * Load the [Content_Types].xml data structures
 * that define content types for the parts in the package.
 */

struct sa_default_s
{
	char *extension;
	char *mimetype;
	sa_default *next;
};

struct sa_override_s
{
	char *partname;
	char *mimetype;
	sa_override *next;
};

static fz_error *
readcontenttypes(sa_package *pack)
{
	fz_error *error;
	fz_stream *zipstm;
	sa_xmlparser *parser;
	sa_xmlitem *item;

	error = sa_openzipentry(&zipstm, pack->zip, "[Content_Types].xml");
	if (error)
		return error;

	error = sa_openxml(&parser, zipstm, 0);
	if (error)
		goto cleanupzip;

	item = sa_xmlnext(parser);
	while (item)
	{
		if (!strcmp(sa_xmlname(item), "Types"))
		{
			sa_xmldown(parser);
			item = sa_xmlnext(parser);
			while (item)
			{
				if (!strcmp(sa_xmlname(item), "Default"))
				{
					char *ext = sa_xmlatt(item, "Extension");
					char *type = sa_xmlatt(item, "ContentType");
					if (ext && type)
					{
						sa_default *newdef;
						if (strchr(type, ';'))
							strchr(type, ';')[0] = 0;
						newdef = fz_malloc(sizeof(sa_default));
						if (!newdef) { error = fz_outofmem; goto cleanupxml; }
						newdef->extension = fz_strdup(ext);
						newdef->mimetype = fz_strdup(type);
						newdef->next = pack->defaults;
						pack->defaults = newdef;
					}
				}

				if (!strcmp(sa_xmlname(item), "Override"))
				{
					char *name = sa_xmlatt(item, "PartName");
					char *type = sa_xmlatt(item, "ContentType");
					if (name && type)
					{
						sa_override *newovr;
						if (strchr(type, ';'))
							strchr(type, ';')[0] = 0;
						newovr = fz_malloc(sizeof(sa_override));
						if (!newovr) { error = fz_outofmem; goto cleanupxml; }
						newovr->partname = fz_strdup(name);
						newovr->mimetype = fz_strdup(type);
						newovr->next = pack->overrides;
						pack->overrides = newovr;
					}
				}

				item = sa_xmlnext(parser);
			}
			sa_xmlup(parser);
		}
	}

	sa_closexml(parser);
	fz_dropstream(zipstm);
	return nil;

cleanupxml:
	sa_closexml(parser);
cleanupzip:
	fz_dropstream(zipstm);
	return error;
}

/*
 * Return the type of a part, or nil if it doenst exist or has no type.
 */
char *
sa_typepart(sa_package *pack, char *partname)
{
	sa_default *def;
	sa_override *ovr;
	char *ext;

	if (partname[0] != '/')
		return nil;

	if (sa_accesszipentry(pack->zip, partname + 1))
	{
		for (ovr = pack->overrides; ovr; ovr = ovr->next)
			if (!sa_strcmp(partname, ovr->partname))
				return ovr->mimetype;

		ext = strrchr(partname, '.');
		if (ext)
		{
			for (def = pack->defaults; def; def = def->next)
				if (!sa_strcmp(ext + 1, def->extension))
					return def->mimetype;
		}
	}

	return nil;
}

/*
 * Open a package...
 * Open the ZIP file.
 * Load the content types.
 * Load the relations for the root.
 */
fz_error *
sa_openpackage(sa_package **packp, char *filename)
{
	fz_error *error;
	sa_package *pack;

	pack = fz_malloc(sizeof(sa_package));
	if (!pack)
		return fz_outofmem;

	pack->zip = nil;
	pack->defaults = nil;
	pack->overrides = nil;

	error = sa_openzip(&pack->zip, filename);
	if (error)
	{
		sa_closepackage(pack);
		return error;
	}

	error = readcontenttypes(pack);
	if (error)
	{
		sa_closepackage(pack);
		return error;
	}

	*packp = pack;
	return nil;
}

void
sa_closepackage(sa_package *pack)
{
	sa_default *def, *ndef;
	sa_override *ovr, *novr;

	if (pack->zip)
		sa_closezip(pack->zip);

	for (def = pack->defaults; def; def = ndef)
	{
		ndef = def->next;
		fz_free(def->extension);
		fz_free(def->mimetype);
		fz_free(def);
	}

	for (ovr = pack->overrides; ovr; ovr = novr)
	{
		novr = ovr->next;
		fz_free(ovr->partname);
		fz_free(ovr->mimetype);
		fz_free(ovr);
	}

	fz_free(pack);
}

void
sa_debugpackage(sa_package *pack)
{
	sa_default *def;
	sa_override *ovr;

	printf("package\n{\n");

	if (pack->zip)
		sa_debugzip(pack->zip);

	printf("  defaults\n  {\n");
	for (def = pack->defaults; def; def = def->next)
		printf("    %-8s %s\n", def->extension, def->mimetype);
	printf("  }\n");

	printf("  overrides\n  {\n");
	for (ovr = pack->overrides; ovr; ovr = ovr->next)
		printf("    %s\n        %s\n", ovr->partname, ovr->mimetype);
	printf("  }\n");

	printf("}\n");
}

/*
 * Open a part for reading.
 * It is NOT safe to open more than one part at a time.
 */
fz_error *
sa_openpart(fz_stream **stmp, sa_package *pack, char *partname)
{
	if (partname[0] != '/')
		return fz_throw("ioerror: invalid part name: %s", partname);
	return sa_openzipentry(stmp, pack->zip, partname + 1);
}

/*
 * Load a linked list of all the relations of a part.
 * This is contained in <folder>/_rels/<file>.rels
 */
fz_error *
sa_loadrelations(sa_relation **relsp, sa_package *pack, char *partname)
{
	fz_error *error;
	fz_stream *zipstm;
	sa_xmlparser *parser;
	sa_xmlitem *item;
	sa_relation *rels;
	int len;
	char *sep;
	char *relsname;
	char buf[1024];

	if (partname[0] != '/')
		return fz_throw("ioerror: invalid part name: %s", partname);

	sep = strrchr(partname, '/');
	if (!sep)
		return fz_throw("ioerror: invalid part name: %s", partname);

	len = strlen(partname) + 11 + 1;
	relsname = fz_malloc(len);
	if (!relsname)
		return fz_outofmem;

	memcpy(relsname, partname, sep - partname + 1);
	relsname[sep - partname + 1] = 0;
	strcat(relsname, "_rels/");
	strcat(relsname, sep + 1);
	strcat(relsname, ".rels");

	rels = nil;

	if (!sa_accesszipentry(pack->zip, relsname + 1))
	{
		fz_free(relsname);
		*relsp = nil;
		return nil;
	}

	error = sa_openzipentry(&zipstm, pack->zip, relsname + 1);
	if (error)
		goto cleanupname;

	error = sa_openxml(&parser, zipstm, 0);
	if (error)
		goto cleanupzip;

	item = sa_xmlnext(parser);
	while (item)
	{
		if (!strcmp(sa_xmlname(item), "Relationships"))
		{
			sa_xmldown(parser);
			item = sa_xmlnext(parser);
			while (item)
			{
				if (!strcmp(sa_xmlname(item), "Relationship"))
				{
					char *mode = sa_xmlatt(item, "TargetMode");
					char *id = sa_xmlatt(item, "Id");
					char *target = sa_xmlatt(item, "Target");
					char *type = sa_xmlatt(item, "Type");

					if (!mode)
						mode = "Internal";

					if (id && target && type)
					{
						sa_relation *newrel;
						newrel = fz_malloc(sizeof(sa_relation));
						if (!newrel) { error = fz_outofmem; goto cleanupxml; }
						newrel->external = !strcmp(mode, "External");
						newrel->id = fz_strdup(id);
						newrel->type = fz_strdup(type);
						newrel->next = rels;
						if (newrel->external)
							newrel->target = fz_strdup(target);
						else
						{
							sa_resolvepath(buf, partname, target, sizeof buf);
							newrel->target = fz_strdup(buf);
						}
						rels = newrel;
					}
				}

				item = sa_xmlnext(parser);
			}
			sa_xmlup(parser);
		}
	}

	sa_closexml(parser);
	fz_dropstream(zipstm);
	fz_free(relsname);
	*relsp = rels;
	return nil;

cleanupxml:
	sa_closexml(parser);
cleanupzip:
	fz_dropstream(zipstm);
cleanupname:
	fz_free(relsname);
	return error;
}

void
sa_debugrelations(sa_relation *rel)
{
	printf("relations\n{\n");
	while (rel)
	{
		printf("  %s\n", rel->type);
		printf("      %s%s\n", rel->external ? "external " : "", rel->target);
		rel = rel->next;
	}
	printf("}\n");
}

void
sa_droprelations(sa_relation *rel)
{
	sa_relation *nrel;
	while (rel)
	{
		nrel = rel->next;
		fz_free(rel->target);
		fz_free(rel->id);
		fz_free(rel->type);
		fz_free(rel);
		rel = nrel;
	}
}

