-- Adding the rc_ip field for logging of IP addresses in recentchanges

ALTER TABLE /*$wgDBprefix*/recentchanges 
  ADD rc_ip varbinary(40) NOT NULL default '',
  ADD INDEX rc_ip (rc_ip);


