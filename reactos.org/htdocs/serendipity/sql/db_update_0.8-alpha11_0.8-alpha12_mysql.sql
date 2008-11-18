ALTER TABLE {PREFIX}authors ADD realname VARCHAR( 255 ) NOT NULL FIRST;
UPDATE {PREFIX}authors SET realname = username;
