
-- Relation table between user and groups
CREATE TABLE /*$wgDBprefix*/user_groups (
	ug_user int unsigned NOT NULL default '0',
	ug_group varbinary(16) NOT NULL default '0',
	PRIMARY KEY  (ug_user,ug_group)
  KEY (ug_group)
) /*$wgDBTableOptions*/;
