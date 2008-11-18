ALTER TABLE {PREFIX}images ADD COLUMN path text;
CREATE {FULLTEXT} INDEX pathkey_idx on {PREFIX}images (path);
