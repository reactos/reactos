create table {PREFIX}groups (
  id {AUTOINCREMENT} {PRIMARY},
  name varchar(64) default null
);

create table {PREFIX}groupconfig (
  id int(10) {UNSIGNED} not null default '0',
  property varchar(64) default null,
  value varchar(128) default null
);

CREATE INDEX groupid_idx ON {PREFIX}groupconfig (id);
CREATE INDEX groupprop_idx ON {PREFIX}groupconfig (id, property);

create table {PREFIX}authorgroups (
  groupid int(10) {UNSIGNED} not null default '0',
  authorid int(10) {UNSIGNED} not null default '0'
);

CREATE INDEX authorgroup_idxA ON {PREFIX}authorgroups (groupid);
CREATE INDEX authorgroup_idxB ON {PREFIX}authorgroups (authorid);
