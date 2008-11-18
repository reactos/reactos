-- For a few generic cache operations if not using Memcached
CREATE TABLE /*$wgDBprefix*/objectcache (
  keyname varbinary(255) NOT NULL default '',
  value mediumblob,
  exptime datetime,
  UNIQUE KEY (keyname),
  KEY (exptime)

) /*$wgDBTableOptions*/;
