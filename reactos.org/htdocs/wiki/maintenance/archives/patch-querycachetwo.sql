-- Used for caching expensive grouped queries that need two links (for example double-redirects)

CREATE TABLE /*$wgDBprefix*/querycachetwo (
  -- A key name, generally the base name of of the special page.
  qcc_type varbinary(32) NOT NULL,
  
  -- Some sort of stored value. Sizes, counts...
  qcc_value int unsigned NOT NULL default '0',
  
  -- Target namespace+title
  qcc_namespace int NOT NULL default '0',
  qcc_title varchar(255) binary NOT NULL default '',
  
  -- Target namespace+title2
  qcc_namespacetwo int NOT NULL default '0',
  qcc_titletwo varchar(255) binary NOT NULL default '',

  KEY qcc_type (qcc_type,qcc_value),
  KEY qcc_title (qcc_type,qcc_namespace,qcc_title),
  KEY qcc_titletwo (qcc_type,qcc_namespacetwo,qcc_titletwo)

) /*$wgDBTableOptions*/;
