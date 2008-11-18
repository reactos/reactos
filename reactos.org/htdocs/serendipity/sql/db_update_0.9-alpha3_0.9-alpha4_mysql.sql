create table {PREFIX}plugincategories (
  class_name varchar(250) default null,
  category varchar(250) default null
);

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
);

CREATE INDEX pluginlist_f_idx ON {PREFIX}pluginlist(plugin_file);
CREATE INDEX pluginlist_cn_idx ON {PREFIX}pluginlist(class_name);
CREATE INDEX pluginlist_pt_idx ON {PREFIX}pluginlist(plugintype);
CREATE INDEX pluginlist_pl_idx ON {PREFIX}pluginlist(pluginlocation);
