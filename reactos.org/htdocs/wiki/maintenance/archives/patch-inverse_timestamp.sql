-- Removes the inverse_timestamp field from early 1.5 alphas.
-- This field was used in the olden days as a crutch for sorting
-- limitations in MySQL 3.x, but is being dropped now as an
-- unnecessary burden. Serious wikis should be running on 4.x.
--
-- Updater added 2005-03-13

ALTER TABLE /*$wgDBprefix*/revision
  DROP COLUMN inverse_timestamp,
  DROP INDEX page_timestamp,
  DROP INDEX user_timestamp,
  DROP INDEX usertext_timestamp,
  ADD  INDEX page_timestamp (rev_page,rev_timestamp),
  ADD  INDEX user_timestamp (rev_user,rev_timestamp),
  ADD  INDEX usertext_timestamp (rev_user_text,rev_timestamp);
