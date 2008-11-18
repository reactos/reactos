-- Protected titles - nonexistent pages that have been protected
CREATE TABLE /*$wgDBprefix*/protected_titles (
  pt_namespace int NOT NULL,
  pt_title varchar(255) binary NOT NULL,
  pt_user int unsigned NOT NULL,
  pt_reason tinyblob,
  pt_timestamp binary(14) NOT NULL,
  pt_expiry varbinary(14) NOT NULL default '',
  pt_create_perm varbinary(60) NOT NULL,
  PRIMARY KEY (pt_namespace,pt_title),
  KEY pt_timestamp (pt_timestamp)
) /*$wgDBTableOptions*/;
