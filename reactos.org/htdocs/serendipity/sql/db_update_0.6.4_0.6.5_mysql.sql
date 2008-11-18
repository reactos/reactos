ALTER TABLE {PREFIX}category
  ADD parentid INT(11) DEFAULT '0' NOT NULL,
  ADD category_left INT(11) DEFAULT '0' NOT NULL,
  ADD category_right INT(11) DEFAULT '0' NOT NULL;

create table {PREFIX}entrycat (
  entryid int(11) not null,
  categoryid int(11) not null
);

INSERT INTO {PREFIX}entrycat (entryid, categoryid) SELECT id, categoryid FROM {PREFIX}entries;

ALTER TABLE {PREFIX}entries DROP categoryid;

ALTER TABLE {PREFIX}comments ADD parent_id int(10) {UNSIGNED} default '0' not null;
