create table {PREFIX}tempauthors (
  realname varchar(255) NOT NULL default '',
  username varchar(20) default null,
  password varchar(32) default null,
  authorid {AUTOINCREMENT} {PRIMARY},
  mail_comments int(1) default '1',
  mail_trackbacks int(1) default '1',
  email varchar(128) not null default '',
  userlevel int(4) {UNSIGNED} not null default '0',
  right_publish int(1) default '1'
);

INSERT INTO {PREFIX}tempauthors (username, password, authorid, mail_comments, mail_trackbacks, email, userlevel, right_publish) SELECT username, password, authorid, mail_comments, mail_trackbacks, email, userlevel, right_publish FROM {PREFIX}authors;
DROP TABLE {PREFIX}authors;

create table {PREFIX}authors (
  realname varchar(255) NOT NULL default '',
  username varchar(20) default null,
  password varchar(32) default null,
  authorid {AUTOINCREMENT} {PRIMARY},
  mail_comments int(1) default '1',
  mail_trackbacks int(1) default '1',
  email varchar(128) not null default '',
  userlevel int(4) {UNSIGNED} not null default '0',
  right_publish int(1) default '1'
);

INSERT INTO {PREFIX}authors (username, password, authorid, mail_comments, mail_trackbacks, email, userlevel, right_publish) SELECT username, password, authorid, mail_comments, mail_trackbacks, email, userlevel, right_publish FROM {PREFIX}tempauthors;
DROP TABLE {PREFIX}tempauthors;

UPDATE {PREFIX}authors SET realname = username;