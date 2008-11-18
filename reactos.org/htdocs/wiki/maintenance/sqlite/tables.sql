CREATE TABLE /*$wgDBprefix*/user (
  user_id INTEGER  PRIMARY KEY AUTOINCREMENT,
  user_name varchar(255)   default '',
  user_real_name varchar(255)   default '',
  user_password tinyblob ,
  user_newpassword tinyblob ,
  user_newpass_time BLOB,
  user_email tinytext ,
  user_options blob ,
  user_touched BLOB  default '',
  user_token BLOB  default '',
  user_email_authenticated BLOB,
  user_email_token BLOB,
  user_email_token_expires BLOB,
  user_registration BLOB,
  user_editcount int) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/user_groups (
  ug_user INTEGER  default '0',
  ug_group varBLOB  default '') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/user_newtalk (
  user_id INTEGER  default '0',
  user_ip varBLOB  default '',
  user_last_timestamp BLOB  default '') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/page (
  page_id INTEGER  PRIMARY KEY AUTOINCREMENT,
  page_namespace INTEGER ,
  page_title varchar(255)  ,
  page_restrictions tinyblob ,
  page_counter bigint  default '0',
  page_is_redirect tinyint  default '0',
  page_is_new tinyint  default '0',
  page_random real ,
  page_touched BLOB  default '',
  page_latest INTEGER ,
  page_len INTEGER ) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/revision (
  rev_id INTEGER  PRIMARY KEY AUTOINCREMENT,
  rev_page INTEGER ,
  rev_text_id INTEGER ,
  rev_comment tinyblob ,
  rev_user INTEGER  default '0',
  rev_user_text varchar(255)   default '',
  rev_timestamp BLOB  default '',
  rev_minor_edit tinyint  default '0',
  rev_deleted tinyint  default '0',
  rev_len int,
  rev_parent_id INTEGER default NULL) /*$wgDBTableOptions*/  ;

CREATE TABLE /*$wgDBprefix*/text (
  old_id INTEGER  PRIMARY KEY AUTOINCREMENT,
  old_text mediumblob ,
  old_flags tinyblob ) /*$wgDBTableOptions*/  ;

CREATE TABLE /*$wgDBprefix*/archive (
  ar_namespace INTEGER  default '0',
  ar_title varchar(255)   default '',
  ar_text mediumblob ,
  ar_comment tinyblob ,
  ar_user INTEGER  default '0',
  ar_user_text varchar(255)  ,
  ar_timestamp BLOB  default '',
  ar_minor_edit tinyint  default '0',
  ar_flags tinyblob ,
  ar_rev_id int,
  ar_text_id int,
  ar_deleted tinyint  default '0',
  ar_len int,
  ar_page_id int,
  ar_parent_id INTEGER default NULL) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/pagelinks (
  pl_from INTEGER  default '0',
  pl_namespace INTEGER  default '0',
  pl_title varchar(255)   default '') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/templatelinks (
  tl_from INTEGER  default '0',
  tl_namespace INTEGER  default '0',
  tl_title varchar(255)   default '') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/imagelinks (
  il_from INTEGER  default '0',
  il_to varchar(255)   default '') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/categorylinks (
  cl_from INTEGER  default '0',
  cl_to varchar(255)   default '',
  cl_sortkey varchar(70)   default '',
  cl_timestamp timestamp ) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/category (
  cat_id INTEGER  PRIMARY KEY AUTOINCREMENT,
  cat_title varchar(255)  ,
  cat_pages INTEGER signed  default 0,
  cat_subcats INTEGER signed  default 0,
  cat_files INTEGER signed  default 0,
  cat_hidden tinyint  default 0) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/externallinks (
  el_from INTEGER  default '0',
  el_to blob ,
  el_index blob ) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/langlinks (
  ll_from INTEGER  default '0',
  ll_lang varBLOB  default '',
  ll_title varchar(255)   default '') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/site_stats (
  ss_row_id INTEGER ,
  ss_total_views bigint default '0',
  ss_total_edits bigint default '0',
  ss_good_articles bigint default '0',
  ss_total_pages bigint default '-1',
  ss_users bigint default '-1',
  ss_admins INTEGER default '-1',
  ss_images INTEGER default '0') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/hitcounter (
  hc_id INTEGER 
)  ;

CREATE TABLE /*$wgDBprefix*/ipblocks (
  ipb_id INTEGER  PRIMARY KEY AUTOINCREMENT,
  ipb_address tinyblob ,
  ipb_user INTEGER  default '0',
  ipb_by INTEGER  default '0',
  ipb_by_text varchar(255)   default '',
  ipb_reason tinyblob ,
  ipb_timestamp BLOB  default '',
  ipb_auto bool  default 0,
  ipb_anon_only bool  default 0,
  ipb_create_account bool  default 1,
  ipb_enable_autoblock bool  default '1',
  ipb_expiry varBLOB  default '',
  ipb_range_start tinyblob ,
  ipb_range_end tinyblob ,
  ipb_deleted bool  default 0,
  ipb_block_email bool  default 0) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/image (
  img_name varchar(255)   default '',
  img_size INTEGER  default '0',
  img_width INTEGER  default '0',
  img_height INTEGER  default '0',
  img_metadata mediumblob ,
  img_bits INTEGER  default '0',
  img_media_type TEXT default NULL,
  img_major_mime TEXT  default "unknown",
  img_minor_mime varBLOB  default "unknown",
  img_description tinyblob ,
  img_user INTEGER  default '0',
  img_user_text varchar(255)  ,
  img_timestamp varBLOB  default '',
  img_sha1 varBLOB  default '') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/oldimage (
  oi_name varchar(255)   default '',
  oi_archive_name varchar(255)   default '',
  oi_size INTEGER  default 0,
  oi_width INTEGER  default 0,
  oi_height INTEGER  default 0,
  oi_bits INTEGER  default 0,
  oi_description tinyblob ,
  oi_user INTEGER  default '0',
  oi_user_text varchar(255)  ,
  oi_timestamp BLOB  default '',
  oi_metadata mediumblob ,
  oi_media_type TEXT default NULL,
  oi_major_mime TEXT  default "unknown",
  oi_minor_mime varBLOB  default "unknown",
  oi_deleted tinyint  default '0',
  oi_sha1 varBLOB  default '') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/filearchive (
  fa_id INTEGER  PRIMARY KEY AUTOINCREMENT,
  fa_name varchar(255)   default '',
  fa_archive_name varchar(255)  default '',
  fa_storage_group varBLOB,
  fa_storage_key varBLOB default '',
  fa_deleted_user int,
  fa_deleted_timestamp BLOB default '',
  fa_deleted_reason text,
  fa_size INTEGER default '0',
  fa_width INTEGER default '0',
  fa_height INTEGER default '0',
  fa_metadata mediumblob,
  fa_bits INTEGER default '0',
  fa_media_type TEXT default NULL,
  fa_major_mime TEXT default "unknown",
  fa_minor_mime varBLOB default "unknown",
  fa_description tinyblob,
  fa_user INTEGER default '0',
  fa_user_text varchar(255) ,
  fa_timestamp BLOB default '',
  fa_deleted tinyint  default '0') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/recentchanges (
  rc_id INTEGER  PRIMARY KEY AUTOINCREMENT,
  rc_timestamp varBLOB  default '',
  rc_cur_time varBLOB  default '',
  rc_user INTEGER  default '0',
  rc_user_text varchar(255)  ,
  rc_namespace INTEGER  default '0',
  rc_title varchar(255)   default '',
  rc_comment varchar(255)   default '',
  rc_minor tinyint  default '0',
  rc_bot tinyint  default '0',
  rc_new tinyint  default '0',
  rc_cur_id INTEGER  default '0',
  rc_this_oldid INTEGER  default '0',
  rc_last_oldid INTEGER  default '0',
  rc_type tinyint  default '0',
  rc_moved_to_ns tinyint  default '0',
  rc_moved_to_title varchar(255)   default '',
  rc_patrolled tinyint  default '0',
  rc_ip varBLOB  default '',
  rc_old_len int,
  rc_new_len int,
  rc_deleted tinyint  default '0',
  rc_logid INTEGER  default '0',
  rc_log_type varBLOB NULL default NULL,
  rc_log_action varBLOB NULL default NULL,
  rc_params blob NULL) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/watchlist (
  wl_user INTEGER ,
  wl_namespace INTEGER  default '0',
  wl_title varchar(255)   default '',
  wl_notificationtimestamp varBLOB) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/math (
  math_inputhash varBLOB ,
  math_outputhash varBLOB ,
  math_html_conservativeness tinyint ,
  math_html text,
  math_mathml text) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/searchindex (
  si_page INTEGER ,
  si_title varchar(255)  default '',
  si_text mediumtext ) ;

CREATE TABLE /*$wgDBprefix*/interwiki (
  iw_prefix varchar(32) ,
  iw_url blob ,
  iw_local bool ,
  iw_trans tinyint  default 0) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/querycache (
  qc_type varBLOB ,
  qc_value INTEGER  default '0',
  qc_namespace INTEGER  default '0',
  qc_title varchar(255)   default '') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/objectcache (
  keyname varBLOB  default '',
  value mediumblob,
  exptime datetime) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/transcache (
  tc_url varBLOB ,
  tc_contents text,
  tc_time INTEGER ) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/logging (
  log_id INTEGER  PRIMARY KEY AUTOINCREMENT,
  log_type varBLOB  default '',
  log_action varBLOB  default '',
  log_timestamp BLOB  default '19700101000000',
  log_user INTEGER  default 0,
  log_namespace INTEGER  default 0,
  log_title varchar(255)   default '',
  log_comment varchar(255)  default '',
  log_params blob ,
  log_deleted tinyint  default '0') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/trackbacks (
  tb_id INTEGER PRIMARY KEY AUTOINCREMENT,
  tb_page INTEGER REFERENCES /*$wgDBprefix*/page(page_id) ON DELETE CASCADE,
  tb_title varchar(255) ,
  tb_url blob ,
  tb_ex text,
  tb_name varchar(255)) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/job (
  job_id INTEGER  PRIMARY KEY AUTOINCREMENT,
  job_cmd varBLOB  default '',
  job_namespace INTEGER ,
  job_title varchar(255)  ,
  job_params blob ) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/querycache_info (
  qci_type varBLOB  default '',
  qci_timestamp BLOB  default '19700101000000') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/redirect (
  rd_from INTEGER  default '0',
  rd_namespace INTEGER  default '0',
  rd_title varchar(255)   default '') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/querycachetwo (
  qcc_type varBLOB ,
  qcc_value INTEGER  default '0',
  qcc_namespace INTEGER  default '0',
  qcc_title varchar(255)   default '',
  qcc_namespacetwo INTEGER  default '0',
  qcc_titletwo varchar(255)   default '') /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/page_restrictions (
  pr_page INTEGER ,
  pr_type varBLOB ,
  pr_level varBLOB ,
  pr_cascade tinyint ,
  pr_user INTEGER NULL,
  pr_expiry varBLOB NULL,
  pr_id INTEGER  PRIMARY KEY AUTOINCREMENT) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/protected_titles (
  pt_namespace INTEGER ,
  pt_title varchar(255)  ,
  pt_user INTEGER ,
  pt_reason tinyblob,
  pt_timestamp BLOB ,
  pt_expiry varBLOB  default '',
  pt_create_perm varBLOB ) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/page_props (
  pp_page INTEGER ,
  pp_propname varBLOB ,
  pp_value blob ) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/updatelog (
  ul_key varchar(255) ) /*$wgDBTableOptions*/;


