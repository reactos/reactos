CREATE TABLE /*$wgDBprefix*/category (
  cat_id int unsigned NOT NULL auto_increment,

  cat_title varchar(255) binary NOT NULL,

  cat_pages int signed NOT NULL default 0,
  cat_subcats int signed NOT NULL default 0,
  cat_files int signed NOT NULL default 0,

  cat_hidden tinyint(1) unsigned NOT NULL default 0,
  
  PRIMARY KEY (cat_id),
  UNIQUE KEY (cat_title),

  KEY (cat_pages)
) /*$wgDBTableOptions*/;

