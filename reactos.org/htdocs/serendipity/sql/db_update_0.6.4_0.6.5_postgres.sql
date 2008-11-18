ALTER TABLE {PREFIX}category ADD COLUMN parentid INT4;
ALTER TABLE {PREFIX}category ADD COLUMN category_left INT4;
ALTER TABLE {PREFIX}category ADD COLUMN category_right INT4;
UPDATE {PREFIX}category SET parentid=0, category_left=0, category_right=0;

ALTER TABLE {PREFIX}category ALTER COLUMN parentid SET NOT NULL;
ALTER TABLE {PREFIX}category ALTER COLUMN category_left SET NOT NULL;
ALTER TABLE {PREFIX}category ALTER COLUMN category_right SET NOT NULL;

CREATE TABLE {PREFIX}entrycat (
  entryid int4 not null,
  categoryid int4 not null
);

INSERT INTO {PREFIX}entrycat (entryid, categoryid)  (SELECT id, categoryid FROM {PREFIX}entries);

ALTER TABLE {PREFIX}entries DROP COLUMN categoryid;

ALTER TABLE {PREFIX}comments ADD COLUMN parent_id int4;
UPDATE {PREFIX}comments SET parent_id=0;
ALTER TABLE {PREFIX}comments ALTER COLUMN parent_id SET NOT NULL;
