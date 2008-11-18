--
-- E-mail confirmation token and expiration timestamp,
-- for verification of e-mail addresses.
--
-- 2005-04-25
--

ALTER TABLE /*$wgDBprefix*/user
  ADD COLUMN user_email_authenticated binary(14),
  ADD COLUMN user_email_token binary(32),
  ADD COLUMN user_email_token_expires binary(14),
  ADD INDEX (user_email_token);
