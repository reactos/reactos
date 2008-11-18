--- This table stores all the IDs of users whose talk
--- page has been changed (the respective row is deleted
--- when the user looks at the page).
--- The respective column in the user table is no longer
--- required and therefore dropped.

CREATE TABLE /*$wgDBprefix*/user_newtalk (
  user_id int NOT NULL default '0',
  user_ip varbinary(40) NOT NULL default '',
  KEY user_id (user_id),
  KEY user_ip (user_ip)
) /*$wgDBTableOptions*/;

INSERT INTO
  /*$wgDBprefix*/user_newtalk (user_id, user_ip)
  SELECT user_id, ''
    FROM user
    WHERE user_newtalk != 0;

ALTER TABLE /*$wgDBprefix*/user DROP COLUMN user_newtalk;
