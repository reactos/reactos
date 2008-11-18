ALTER TABLE {PREFIX}images ADD COLUMN oldmime varchar(15);
UPDATE {PREFIX}images SET oldmime = mime;
ALTER TABLE {PREFIX}images DROP mime;

ALTER TABLE {PREFIX}images ADD COLUMN mime varchar(255);
UPDATE {PREFIX}images SET mime = oldmime;
ALTER TABLE {PREFIX}images DROP COLUMN oldmime;
ALTER TABLE {PREFIX}images ALTER COLUMN mime SET NOT NULL;

