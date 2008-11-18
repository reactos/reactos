####
#### Important notes:
#### If you make modifications (additions, subtractions), please
#### make the appropriate change in the db_update files.  If you don't
#### know the syntax for a different database, e-mail the list for
#### help.
####
####

#
# Table structure for table '{PREFIX}authors'
#

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
) {UTF_8};

create table {PREFIX}groups (
  id {AUTOINCREMENT} {PRIMARY},
  name varchar(64) default null
) {UTF_8};

create table {PREFIX}groupconfig (
  id int(10) {UNSIGNED} not null default '0',
  property varchar(64) default null,
  value varchar(128) default null
) {UTF_8};

CREATE INDEX groupid_idx ON {PREFIX}groupconfig (id);
CREATE INDEX groupprop_idx ON {PREFIX}groupconfig (id, property);

create table {PREFIX}authorgroups (
  groupid int(10) {UNSIGNED} not null default '0',
  authorid int(10) {UNSIGNED} not null default '0'
) {UTF_8};

CREATE INDEX authorgroup_idxA ON {PREFIX}authorgroups (groupid);
CREATE INDEX authorgroup_idxB ON {PREFIX}authorgroups (authorid);

create table {PREFIX}access (
  groupid int(10) {UNSIGNED} not null default '0',
  artifact_id int(10) {UNSIGNED} not null default '0',
  artifact_type varchar(64) NOT NULL default '',
  artifact_mode varchar(64) NOT NULL default '',
  artifact_index varchar(64) NOT NULL default ''
) {UTF_8};

CREATE INDEX accessgroup_idx ON {PREFIX}access(groupid);
CREATE INDEX accessgroupT_idx ON {PREFIX}access(artifact_id,artifact_type,artifact_mode);
CREATE INDEX accessforeign_idx ON {PREFIX}access(artifact_id);


#
# table structure for table '{PREFIX}comments'
#

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
) {UTF_8};

CREATE INDEX commentry_idx ON {PREFIX}comments (entry_id);
CREATE INDEX commpentry_idx ON {PREFIX}comments (parent_id);
CREATE INDEX commtype_idx ON {PREFIX}comments (type);
CREATE INDEX commstat_idx ON {PREFIX}comments (status);

#
# table structure for table '{PREFIX}entries'
#

create table {PREFIX}entries (
  id {AUTOINCREMENT} {PRIMARY},
  title varchar(200) default null,
  timestamp int(10) {UNSIGNED} default null,
  body text,
  comments int(4) {UNSIGNED} default '0',
  trackbacks int(4) {UNSIGNED} default '0',
  extended text,
  exflag int(1) default null,
  author varchar(20) default null,
  authorid int(11) default null,
  isdraft {BOOLEAN},
  allow_comments {BOOLEAN},
  last_modified int(10) {UNSIGNED} default null,
  moderate_comments {BOOLEAN}
) {UTF_8};

# FULLTEXT_MYSQL is ignored on all Non-MySQL setups (SQLite, PostgreSQL)
CREATE {FULLTEXT_MYSQL} INDEX entry_idx on {PREFIX}entries (title,body,extended);
CREATE INDEX date_idx ON {PREFIX}entries (timestamp);
CREATE INDEX mod_idx ON {PREFIX}entries (last_modified);
CREATE INDEX edraft_idx ON {PREFIX}entries (isdraft);
CREATE INDEX eauthor_idx ON {PREFIX}entries (authorid);

#
# table structure for table '{PREFIX}references'
#

create table {PREFIX}references (
  id {AUTOINCREMENT} {PRIMARY},
  entry_id int(10) {UNSIGNED} not null default '0',
  link text,
  name text
) {UTF_8};

CREATE INDEX refentry_idx ON {PREFIX}references (entry_id);

#
# Table structure for table '{PREFIX}exits'
#

CREATE TABLE {PREFIX}exits (
  entry_id int(11) NOT NULL default '0',
  day date NOT NULL,
  count int(11) NOT NULL default '0',
  scheme varchar(5),
  host varchar(128) NOT NULL,
  port varchar(5),
  path varchar(255),
  query varchar(255),
  PRIMARY KEY  (host,day,entry_id)
) {UTF_8};

CREATE INDEX exits_idx ON {PREFIX}exits (entry_id,day);

#
# Table structure for table '{PREFIX}referrers'
#

CREATE TABLE {PREFIX}referrers (
  entry_id int(11) NOT NULL default '0',
  day date NOT NULL,
  count int(11) NOT NULL default '0',
  scheme varchar(5),
  host varchar(128) NOT NULL,
  port varchar(5),
  path varchar(255),
  query varchar(255),
  PRIMARY KEY  (host,day,entry_id)
) {UTF_8};

CREATE INDEX referrers_idx ON {PREFIX}referrers (entry_id,day);

#
# Table structure for table 'serendipity_config'
#

create table {PREFIX}config (
  name varchar(255) not null,
  value text not null,
  authorid int(11) default '0'
) {UTF_8};

CREATE INDEX configauthorid_idx ON {PREFIX}config (authorid);

CREATE TABLE {PREFIX}suppress (
  ip varchar(15) default NULL,
  scheme varchar(5),
  host varchar(128),
  port varchar(5),
  path varchar(255),
  query varchar(255),
  last timestamp NOT NULL
) {UTF_8};

CREATE INDEX url_idx on {PREFIX}suppress (host, ip);
CREATE INDEX urllast_idx on {PREFIX}suppress (last);

CREATE TABLE {PREFIX}plugins (
  name varchar(128) not null,
  placement varchar(6) not null default 'right',
  sort_order int(4) not null default '0',
  authorid int(11) default '0',
  path varchar(255) default null,
  PRIMARY KEY(name)
) {UTF_8};

CREATE INDEX pluginauthorid_idx ON {PREFIX}plugins (authorid);
CREATE INDEX pluginplace_idx ON {PREFIX}plugins (placement);

CREATE TABLE {PREFIX}category (
  categoryid {AUTOINCREMENT} {PRIMARY},
  category_name varchar(255) default NULL,
  category_icon varchar(255) default NULL,
  category_description text,
  authorid int(11) default NULL,
  category_left int(11) default '0',
  category_right int(11) default '0',
  parentid int(11) DEFAULT '0' NOT NULL
) {UTF_8};

CREATE INDEX categorya_idx ON {PREFIX}category (authorid);
CREATE INDEX categoryp_idx ON {PREFIX}category (parentid);
CREATE INDEX categorylr_idx ON {PREFIX}category (category_left, category_right);

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
) {UTF_8};

CREATE INDEX imagesauthorid_idx ON {PREFIX}images (authorid);
CREATE {FULLTEXT} INDEX pathkey_idx on {PREFIX}images (path);

CREATE TABLE {PREFIX}entrycat (
  entryid int(11) not null,
  categoryid int(11) not null
) {UTF_8};

CREATE UNIQUE INDEX entryid_idx ON {PREFIX}entrycat (entryid, categoryid);

create table {PREFIX}entryproperties (
  entryid int(11) not null,
  property varchar(255) not null,
  value text
) {UTF_8};

CREATE INDEX entrypropid_idx ON {PREFIX}entryproperties (entryid);
CREATE UNIQUE INDEX prop_idx ON {PREFIX}entryproperties (entryid, property);

CREATE TABLE {PREFIX}permalinks (
    permalink varchar(255) not null default '',
    entry_id int(10) {UNSIGNED} not null default '0',
    type varchar(200) not null default '',
    data text
) {UTF_8};

CREATE INDEX pl_idx ON {PREFIX}permalinks (permalink);
CREATE INDEX ple_idx ON {PREFIX}permalinks (entry_id);
CREATE INDEX plt_idx ON {PREFIX}permalinks (type);
CREATE INDEX plcomb_idx ON {PREFIX}permalinks (permalink, type);

create table {PREFIX}plugincategories (
  class_name varchar(250) default null,
  category varchar(250) default null
) {UTF_8};

CREATE INDEX plugincat_idx ON {PREFIX}plugincategories(class_name, category);

create table {PREFIX}pluginlist (
  plugin_file varchar(255) NOT NULL default '',
  class_name varchar(255) NOT NULL default '',
  plugin_class varchar(255) NOT NULL default '',
  pluginPath varchar(255) NOT NULL default '',
  name varchar(255) NOT NULL default '',
  description text NOT NULL,
  version varchar(12) NOT NULL default '',
  upgrade_version varchar(12) NOT NULL default '',
  plugintype varchar(255) NOT NULL default '',
  pluginlocation varchar(255) NOT NULL default '',
  stackable int(1) NOT NULL default '0',
  author varchar(255) NOT NULL default '',
  requirements text NOT NULL,
  website varchar(255) NOT NULL default '',
  last_modified int(11) NOT NULL default '0'
) {UTF_8};

CREATE INDEX pluginlist_f_idx ON {PREFIX}pluginlist(plugin_file);
CREATE INDEX pluginlist_cn_idx ON {PREFIX}pluginlist(class_name);
CREATE INDEX pluginlist_pt_idx ON {PREFIX}pluginlist(plugintype);
CREATE INDEX pluginlist_pl_idx ON {PREFIX}pluginlist(pluginlocation);
