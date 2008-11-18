CREATE TABLE /*$wgDBprefix*/user (
  user_id int NOT NULL IDENTITY(1,1),
  user_name varchar(255) NOT NULL default '',
  user_real_name varchar(255) NOT NULL default '',
  user_password text NOT NULL,
  user_newpassword text NOT NULL,
  user_newpass_time varchar(5) NULL,
  user_email text NOT NULL,
  user_options text NOT NULL,
  user_touched varchar(5) NOT NULL default '',
  user_token varchar(10) NOT NULL default '',
  user_email_authenticated varchar(5) NULL,
  user_email_token varchar(10) NULL,
  user_email_token_expires varchar(5) NULL,
  user_registration varchar(5) NULL,
  user_editcount int,
  PRIMARY KEY (user_id)
);

CREATE TABLE /*$wgDBprefix*/user_groups (
  ug_user int NOT NULL default '0',
  ug_group varchar(5) NOT NULL default '',
  PRIMARY KEY (ug_user,ug_group)
);

CREATE TABLE /*$wgDBprefix*/user_newtalk (
  user_id int NOT NULL default '0',
  user_ip varchar(13) NOT NULL default '',
  user_last_timestamp varchar(5) NOT NULL default ''
);

CREATE TABLE /*$wgDBprefix*/page (
  page_id int NOT NULL IDENTITY(1,1),
  page_namespace int NOT NULL,
  page_title varchar(255) NOT NULL,
  page_restrictions text NOT NULL,
  page_counter bigint NOT NULL default '0',
  page_is_redirect tinyint NOT NULL default '0',
  page_is_new tinyint NOT NULL default '0',
  page_random real NOT NULL,
  page_touched varchar(5) NOT NULL default '',
  page_latest int NOT NULL,
  page_len int NOT NULL,
  PRIMARY KEY (page_id)
);

CREATE TABLE /*$wgDBprefix*/revision (
  rev_id int NOT NULL IDENTITY(1,1),
  rev_page int NOT NULL,
  rev_text_id int NOT NULL,
  rev_comment text NOT NULL,
  rev_user int NOT NULL default '0',
  rev_user_text varchar(255) NOT NULL default '',
  rev_timestamp varchar(5) NOT NULL default '',
  rev_minor_edit tinyint NOT NULL default '0',
  rev_deleted tinyint NOT NULL default '0',
  rev_len int,
  rev_parent_id int default NULL,
  PRIMARY KEY (rev_page, rev_id)
);

CREATE TABLE /*$wgDBprefix*/text (
  old_id int NOT NULL IDENTITY(1,1),
  old_text text NOT NULL,
  old_flags text NOT NULL,
  PRIMARY KEY (old_id)
);

CREATE TABLE /*$wgDBprefix*/archive (
  ar_namespace int NOT NULL default '0',
  ar_title varchar(255) NOT NULL default '',
  ar_text text NOT NULL,
  ar_comment text NOT NULL,
  ar_user int NOT NULL default '0',
  ar_user_text varchar(255) NOT NULL,
  ar_timestamp varchar(5) NOT NULL default '',
  ar_minor_edit tinyint NOT NULL default '0',
  ar_flags text NOT NULL,
  ar_rev_id int,
  ar_text_id int,
  ar_deleted tinyint NOT NULL default '0',
  ar_len int,
  ar_page_id int,
  ar_parent_id int default NULL
);

CREATE TABLE /*$wgDBprefix*/pagelinks (
  pl_from int NOT NULL default '0',
  pl_namespace int NOT NULL default '0',
  pl_title varchar(255) NOT NULL default ''
);

CREATE TABLE /*$wgDBprefix*/templatelinks (
  tl_from int NOT NULL default '0',
  tl_namespace int NOT NULL default '0',
  tl_title varchar(255) NOT NULL default ''
);

CREATE TABLE /*$wgDBprefix*/imagelinks (
  il_from int NOT NULL default '0',
  il_to varchar(255) NOT NULL default ''
);

CREATE TABLE /*$wgDBprefix*/categorylinks (
  cl_from int NOT NULL default '0',
  cl_to varchar(255) NOT NULL default '',
  cl_sortkey varchar(70) NOT NULL default '',
  cl_timestamp timestamp NOT NULL
);

CREATE TABLE /*$wgDBprefix*/category (
  cat_id int NOT NULL IDENTITY(1,1),
  cat_title varchar(255) NOT NULL,
  cat_pages int NOT NULL default 0,
  cat_subcats int NOT NULL default 0,
  cat_files int NOT NULL default 0,
  cat_hidden tinyint NOT NULL default 0,
  PRIMARY KEY (cat_id)
);

CREATE TABLE /*$wgDBprefix*/externallinks (
  el_from int NOT NULL default '0',
  el_to text NOT NULL,
  el_index text NOT NULL
);

CREATE TABLE /*$wgDBprefix*/langlinks (
  ll_from int NOT NULL default '0',
  ll_lang varchar(7) NOT NULL default '',
  ll_title varchar(255) NOT NULL default ''
);

CREATE TABLE /*$wgDBprefix*/site_stats (
  ss_row_id int NOT NULL,
  ss_total_views bigint default '0',
  ss_total_edits bigint default '0',
  ss_good_articles bigint default '0',
  ss_total_pages bigint default '-1',
  ss_users bigint default '-1',
  ss_admins int default '-1',
  ss_images int default '0'
);

CREATE TABLE /*$wgDBprefix*/hitcounter (
  hc_id int NOT NULL
);

CREATE TABLE /*$wgDBprefix*/ipblocks (
  ipb_id int NOT NULL IDENTITY(1,1),
  ipb_address text NOT NULL,
  ipb_user int NOT NULL default '0',
  ipb_by int NOT NULL default '0',
  ipb_by_text varchar(255) NOT NULL default '',
  ipb_reason text NOT NULL,
  ipb_timestamp varchar(5) NOT NULL default '',
  ipb_auto bit NOT NULL default 0,
  ipb_anon_only bit NOT NULL default 0,
  ipb_create_account bit NOT NULL default 1,
  ipb_enable_autoblock bit NOT NULL default '1',
  ipb_expiry varchar(5) NOT NULL default '',
  ipb_range_start text NOT NULL,
  ipb_range_end text NOT NULL,
  ipb_deleted bit NOT NULL default 0,
  ipb_block_email bit NOT NULL default 0,
  PRIMARY KEY (ipb_id)
);

CREATE TABLE /*$wgDBprefix*/image (
  img_name varchar(255) NOT NULL default '',
  img_size int NOT NULL default '0',
  img_width int NOT NULL default '0',
  img_height int NOT NULL default '0',
  img_metadata text NOT NULL,
  img_bits int NOT NULL default '0',
  img_media_type TEXT default NULL,
  img_major_mime TEXT NOT NULL default "unknown",
  img_minor_mime varchar(10) NOT NULL default "unknown",
  img_description text NOT NULL,
  img_user int NOT NULL default '0',
  img_user_text varchar(255) NOT NULL,
  img_timestamp varchar(5) NOT NULL default '',
  img_sha1 varchar(10) NOT NULL default '',
  PRIMARY KEY (img_name)
);

CREATE TABLE /*$wgDBprefix*/oldimage (
  oi_name varchar(255) NOT NULL default '',
  oi_archive_name varchar(255) NOT NULL default '',
  oi_size int NOT NULL default 0,
  oi_width int NOT NULL default 0,
  oi_height int NOT NULL default 0,
  oi_bits int NOT NULL default 0,
  oi_description text NOT NULL,
  oi_user int NOT NULL default '0',
  oi_user_text varchar(255) NOT NULL,
  oi_timestamp varchar(5) NOT NULL default '',
  oi_metadata text NOT NULL,
  oi_media_type TEXT default NULL,
  oi_major_mime TEXT NOT NULL default "unknown",
  oi_minor_mime varchar(10) NOT NULL default "unknown",
  oi_deleted tinyint NOT NULL default '0',
  oi_sha1 varchar(10) NOT NULL default ''
);

CREATE TABLE /*$wgDBprefix*/filearchive (
  fa_id int NOT NULL IDENTITY(1,1),
  fa_name varchar(255) NOT NULL default '',
  fa_archive_name varchar(255) NULL default '',
  fa_storage_group varchar(5) NULL,
  fa_storage_key varchar(17) NULL default '',
  fa_deleted_user int,
  fa_deleted_timestamp varchar(5) NULL default '',
  fa_deleted_reason text,
  fa_size int default '0',
  fa_width int default '0',
  fa_height int default '0',
  fa_metadata text,
  fa_bits int default '0',
  fa_media_type TEXT default NULL,
  fa_major_mime TEXT default "unknown",
  fa_minor_mime varchar(10) NULL default "unknown",
  fa_description text,
  fa_user int default '0',
  fa_user_text varchar(255) NULL,
  fa_timestamp varchar(5) NULL default '',
  fa_deleted tinyint NOT NULL default '0',
  PRIMARY KEY (fa_id)
);

CREATE TABLE /*$wgDBprefix*/recentchanges (
  rc_id int NOT NULL IDENTITY(1,1),
  rc_timestamp varchar(5) NOT NULL default '',
  rc_cur_time varchar(5) NOT NULL default '',
  rc_user int NOT NULL default '0',
  rc_user_text varchar(255) NOT NULL,
  rc_namespace int NOT NULL default '0',
  rc_title varchar(255) NOT NULL default '',
  rc_comment varchar(255) NOT NULL default '',
  rc_minor tinyint NOT NULL default '0',
  rc_bot tinyint NOT NULL default '0',
  rc_new tinyint NOT NULL default '0',
  rc_cur_id int NOT NULL default '0',
  rc_this_oldid int NOT NULL default '0',
  rc_last_oldid int NOT NULL default '0',
  rc_type tinyint NOT NULL default '0',
  rc_moved_to_ns tinyint NOT NULL default '0',
  rc_moved_to_title varchar(255) NOT NULL default '',
  rc_patrolled tinyint NOT NULL default '0',
  rc_ip varchar(13) NOT NULL default '',
  rc_old_len int,
  rc_new_len int,
  rc_deleted tinyint NOT NULL default '0',
  rc_logid int NOT NULL default '0',
  rc_log_type varchar(17) NULL default NULL,
  rc_log_action varchar(17) NULL default NULL,
  rc_params text NULL,
  PRIMARY KEY (rc_id)
);

CREATE TABLE /*$wgDBprefix*/watchlist (
  wl_user int NOT NULL,
  wl_namespace int NOT NULL default '0',
  wl_title varchar(255) NOT NULL default '',
  wl_notificationtimestamp varchar(5) NULL
);

CREATE TABLE /*$wgDBprefix*/math (
  math_inputhash varchar(5) NOT NULL,
  math_outputhash varchar(5) NOT NULL,
  math_html_conservativeness tinyint NOT NULL,
  math_html text,
  math_mathml text
);

CREATE TABLE /*$wgDBprefix*/searchindex (
  si_page int NOT NULL,
  si_title varchar(255) NOT NULL default '',
  si_text text NOT NULL
);

CREATE TABLE /*$wgDBprefix*/interwiki (
  iw_prefix varchar(32) NOT NULL,
  iw_url text NOT NULL,
  iw_local bit NOT NULL,
  iw_trans tinyint NOT NULL default 0
);

CREATE TABLE /*$wgDBprefix*/querycache (
  qc_type varchar(10) NOT NULL,
  qc_value int NOT NULL default '0',
  qc_namespace int NOT NULL default '0',
  qc_title varchar(255) NOT NULL default ''
);

CREATE TABLE /*$wgDBprefix*/objectcache (
  keyname varchar(17) NOT NULL default '',
  value text,
  exptime datetime
);

CREATE TABLE /*$wgDBprefix*/transcache (
  tc_url varchar(17) NOT NULL,
  tc_contents text,
  tc_time int NOT NULL
);

CREATE TABLE /*$wgDBprefix*/logging (
  log_id int NOT NULL IDENTITY(1,1),
  log_type varchar(4) NOT NULL default '',
  log_action varchar(4) NOT NULL default '',
  log_timestamp varchar(5) NOT NULL default '19700101000000',
  log_user int NOT NULL default 0,
  log_namespace int NOT NULL default 0,
  log_title varchar(255) NOT NULL default '',
  log_comment varchar(255) NOT NULL default '',
  log_params text NOT NULL,
  log_deleted tinyint NOT NULL default '0',
  PRIMARY KEY (log_id)
);

CREATE TABLE /*$wgDBprefix*/trackbacks (
  tb_id int IDENTITY(1,1),
  tb_page int REFERENCES /*$wgDBprefix*/page(page_id) ON DELETE CASCADE,
  tb_title varchar(255) NOT NULL,
  tb_url text NOT NULL,
  tb_ex text,
  tb_name varchar(255) NULL,
  PRIMARY KEY (tb_id)
);

CREATE TABLE /*$wgDBprefix*/job (
  job_id int NOT NULL IDENTITY(1,1),
  job_cmd varchar(17) NOT NULL default '',
  job_namespace int NOT NULL,
  job_title varchar(255) NOT NULL,
  job_params text NOT NULL,
  PRIMARY KEY (job_id)
);

CREATE TABLE /*$wgDBprefix*/querycache_info (
  qci_type varchar(10) NOT NULL default '',
  qci_timestamp varchar(5) NOT NULL default '19700101000000'
);

CREATE TABLE /*$wgDBprefix*/redirect (
  rd_from int NOT NULL default '0',
  rd_namespace int NOT NULL default '0',
  rd_title varchar(255) NOT NULL default '',
  PRIMARY KEY (rd_from)
);

CREATE TABLE /*$wgDBprefix*/querycachetwo (
  qcc_type varchar(10) NOT NULL,
  qcc_value int NOT NULL default '0',
  qcc_namespace int NOT NULL default '0',
  qcc_title varchar(255) NOT NULL default '',
  qcc_namespacetwo int NOT NULL default '0',
  qcc_titletwo varchar(255) NOT NULL default ''
);

CREATE TABLE /*$wgDBprefix*/page_restrictions (
  pr_page int NOT NULL,
  pr_type varchar(17) NOT NULL,
  pr_level varchar(17) NOT NULL,
  pr_cascade tinyint NOT NULL,
  pr_user int NULL,
  pr_expiry varchar(5) NULL,
  pr_id int NOT NULL IDENTITY(1,1),
  PRIMARY KEY (pr_page,pr_type)
);

CREATE TABLE /*$wgDBprefix*/protected_titles (
  pt_namespace int NOT NULL,
  pt_title varchar(255) NOT NULL,
  pt_user int NOT NULL,
  pt_reason text,
  pt_timestamp varchar(5) NOT NULL,
  pt_expiry varchar(5) NOT NULL default '',
  pt_create_perm varchar(17) NOT NULL,
  PRIMARY KEY (pt_namespace,pt_title)
);

CREATE TABLE /*$wgDBprefix*/page_props (
  pp_page int NOT NULL,
  pp_propname varchar(17) NOT NULL,
  pp_value text NOT NULL,
  PRIMARY KEY (pp_page,pp_propname)
);

CREATE TABLE /*$wgDBprefix*/updatelog (
  ul_key varchar(255) NOT NULL,
  PRIMARY KEY (ul_key)
);


