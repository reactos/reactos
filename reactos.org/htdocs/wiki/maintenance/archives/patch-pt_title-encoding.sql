-- pt_title was accidentally left with the wrong collation.
-- This might cause failures with JOINs, and could protect the wrong pages
-- with different case variants or unrelated UTF-8 chars.
ALTER TABLE /*$wgDBprefix*/protected_titles
  CHANGE COLUMN pt_title pt_title varchar(255) binary NOT NULL;
