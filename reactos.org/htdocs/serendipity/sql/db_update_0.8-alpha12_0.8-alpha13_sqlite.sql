CREATE TABLE {PREFIX}tempimages (
  id {AUTOINCREMENT} {PRIMARY},
  name varchar(255) not null default '',
  extension varchar(5) not null default '',
  mime varchar(255) not null default '',
  size int(11) not null default '0',
  dimensions_width int(11) not null default '0',
  dimensions_height int(11) not null default '0',
  date int(11) not null default '0',
  thumbnail_name varchar(255) not null default '',
  authorid int(11) default '0',
  path text,
  hotlink int(1)
);

INSERT INTO {PREFIX}tempimages (id, name, extension, mime, size, dimensions_width, dimensions_height, date, thumbnail_name, authorid, path) SELECT id, name, extension, mime, size, dimensions_width, dimensions_height, date, thumbnail_name, authorid, path FROM {PREFIX}images;
DROP TABLE {PREFIX}images;

CREATE TABLE {PREFIX}images (
  id {AUTOINCREMENT} {PRIMARY},
  name varchar(255) not null default '',
  extension varchar(5) not null default '',
  mime varchar(255) not null default '',
  size int(11) not null default '0',
  dimensions_width int(11) not null default '0',
  dimensions_height int(11) not null default '0',
  date int(11) not null default '0',
  thumbnail_name varchar(255) not null default '',
  authorid int(11) default '0',
  path text,
  hotlink int(1)
);

INSERT INTO {PREFIX}images (id, name, extension, mime, size, dimensions_width, dimensions_height, date, thumbnail_name, authorid, path) SELECT id, name, extension, mime, size, dimensions_width, dimensions_height, date, thumbnail_name, authorid, path FROM {PREFIX}tempimages;
DROP TABLE {PREFIX}tempimages;
