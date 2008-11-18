create table {PREFIX}tempcomments (
  id {AUTOINCREMENT} {PRIMARY},
  entry_id int(10) {UNSIGNED} not null default '0',
  parent_id int(10) {UNSIGNED} not null default '0',
  timestamp int(10) {UNSIGNED} default null,
  title varchar(150) default null,
  author varchar(80) default null,
  email varchar(200) default null,
  url varchar(200) default null,
  ip varchar(15) default null,
  body text,
  type varchar(100) default 'regular',
  subscribed {BOOLEAN},
  status varchar(50) not null
);

INSERT INTO {PREFIX}tempcomments (id, entry_id, parent_id, timestamp, title, author, email, url, ip, body, type, subscribed, status) SELECT id, entry_id, parent_id, timestamp, title, author, email, url, ip, body, type, subscribed, status FROM {PREFIX}comments;
DROP TABLE {PREFIX}comments;

create table {PREFIX}comments (
  id {AUTOINCREMENT} {PRIMARY},
  entry_id int(10) {UNSIGNED} not null default '0',
  parent_id int(10) {UNSIGNED} not null default '0',
  timestamp int(10) {UNSIGNED} default null,
  title varchar(150) default null,
  author varchar(80) default null,
  email varchar(200) default null,
  url varchar(200) default null,
  ip varchar(15) default null,
  body text,
  type varchar(100) default 'regular',
  subscribed {BOOLEAN},
  status varchar(50) not null,
  referer varchar(200) default null
);

INSERT INTO {PREFIX}comments (id, entry_id, parent_id, timestamp, title, author, email, url, ip, body, type, subscribed, status) SELECT id, entry_id, parent_id, timestamp, title, author, email, url, ip, body, type, subscribed, status FROM {PREFIX}tempcomments;
DROP TABLE {PREFIX}tempcomments;
