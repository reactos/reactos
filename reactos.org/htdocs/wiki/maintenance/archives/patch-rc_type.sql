-- recentchanges improvements --

ALTER TABLE /*$wgDBprefix*/recentchanges
  ADD rc_type tinyint unsigned NOT NULL default '0',
  ADD rc_moved_to_ns tinyint unsigned NOT NULL default '0',
  ADD rc_moved_to_title varchar(255) binary NOT NULL default '';

UPDATE /*$wgDBprefix*/recentchanges SET rc_type=1 WHERE rc_new;
UPDATE /*$wgDBprefix*/recentchanges SET rc_type=3 WHERE rc_namespace=4 AND (rc_title='Deletion_log' OR rc_title='Upload_log');
