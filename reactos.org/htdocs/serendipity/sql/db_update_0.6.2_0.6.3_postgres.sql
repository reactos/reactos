ALTER TABLE {PREFIX}images ADD COLUMN path text;
ALTER TABLE {PREFIX}images ALTER COLUMN path SET DEFAULT '/';
UPDATE {PREFIX}images SET path = '/';
CREATE {FULLTEXT} INDEX pathkey_idx on {PREFIX}images (path);
