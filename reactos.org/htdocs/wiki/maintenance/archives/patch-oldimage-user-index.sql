-- 
-- oldimage-user-index.sql
-- 
-- Add user/timestamp index to old image versions
-- 

ALTER TABLE /*$wgDBprefix*/oldimage
   ADD INDEX oi_usertext_timestamp (oi_user_text,oi_timestamp);
