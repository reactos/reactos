-- Add an index to recentchanges on rc_user_text
--
-- Added 2006-11-08
--

     ALTER TABLE /*$wgDBprefix*/recentchanges
ADD INDEX rc_user_text(rc_user_text, rc_timestamp);