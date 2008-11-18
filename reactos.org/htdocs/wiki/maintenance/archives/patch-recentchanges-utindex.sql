--- July 2006
--- Index on recentchanges.( rc_namespace, rc_user_text )
--- Helps the username filtering in Special:Newpages
ALTER TABLE /*$wgDBprefix*/recentchanges ADD INDEX `rc_ns_usertext` ( `rc_namespace` , `rc_user_text` );