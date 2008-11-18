-- For article validation

DROP TABLE IF EXISTS /*$wgDBprefix*/validate;
CREATE TABLE /*$wgDBprefix*/validate (
  `val_user` int(11) NOT NULL default '0',
  `val_page` int(11) unsigned NOT NULL default '0',
  `val_revision` int(11) unsigned NOT NULL default '0',
  `val_type` int(11) unsigned NOT NULL default '0',
  `val_value` int(11) default '0',
  `val_comment` varchar(255) NOT NULL default '',
  `val_ip` varchar(20) NOT NULL default '',
  KEY `val_user` (`val_user`,`val_revision`)
) /*$wgDBTableOptions*/;
