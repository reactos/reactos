@CREATE {FULLTEXT_MYSQL} INDEX entry_idx ON {PREFIX}entries (title,body,extended);
@CREATE INDEX date_idx ON {PREFIX}entries (timestamp);
@CREATE INDEX mod_idx ON {PREFIX}entries (last_modified);
@CREATE INDEX configauthorid_idx ON {PREFIX}config (authorid);
@CREATE INDEX pluginauthorid_idx ON {PREFIX}plugins (authorid);
@CREATE INDEX imagesauthorid_idx ON {PREFIX}images (authorid);
@CREATE INDEX entrypropid_idx ON {PREFIX}entryproperties (entryid);
