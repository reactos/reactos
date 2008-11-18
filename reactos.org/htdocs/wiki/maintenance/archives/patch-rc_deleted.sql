-- Adding rc_deleted field for revisiondelete
-- Add rc_logid to match log_id
ALTER TABLE /*$wgDBprefix*/recentchanges 
  ADD rc_deleted tinyint unsigned NOT NULL default '0',
  ADD rc_logid int unsigned NOT NULL default '0',
  ADD rc_log_type varbinary(255) NULL default NULL,
  ADD rc_log_action varbinary(255) NULL default NULL,
  ADD rc_params BLOB NULL;
