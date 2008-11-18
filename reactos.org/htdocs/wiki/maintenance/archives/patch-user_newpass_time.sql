-- Timestamp of the last time when a new password was
-- sent, for throttling purposes
ALTER TABLE /*$wgDBprefix*/user ADD user_newpass_time binary(14);

