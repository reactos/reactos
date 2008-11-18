-- For getting diff since last view
ALTER TABLE /*$wgDBprefix*/user_newtalk
  ADD user_last_timestamp binary(14) NOT NULL default '';
