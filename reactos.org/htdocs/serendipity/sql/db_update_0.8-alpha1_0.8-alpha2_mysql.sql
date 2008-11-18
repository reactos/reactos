create table {PREFIX}entryproperties (
  entryid int(11) not null,
  property varchar(255) not null,
  value text
);

CREATE UNIQUE INDEX prop_idx ON {PREFIX}entryproperties (entryid, property);
CREATE INDEX entrypropid_idx ON {PREFIX}entryproperties (entryid);
