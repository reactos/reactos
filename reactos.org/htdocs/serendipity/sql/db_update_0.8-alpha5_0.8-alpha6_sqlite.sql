create table {PREFIX}tempplugins (
  name varchar(128) not null,
  placement varchar(6) not null default 'right',
  sort_order int(4) not null default '0',
  authorid int(11) default '0',
  PRIMARY KEY(name)
);

INSERT INTO {PREFIX}tempplugins (name, placement, sort_order, authorid) SELECT name, placement, sort_order, authorid FROM {PREFIX}plugins;
DROP TABLE {PREFIX}plugins;

create table {PREFIX}plugins (
  name varchar(128) not null,
  placement varchar(6) not null default 'right',
  sort_order int(4) not null default '0',
  authorid int(11) default '0',
  path varchar(255) default null,
  PRIMARY KEY(name)
);

INSERT INTO {PREFIX}plugins (name, placement, sort_order, authorid) SELECT name, placement, sort_order, authorid FROM {PREFIX}tempplugins;
DROP TABLE {PREFIX}tempplugins;
