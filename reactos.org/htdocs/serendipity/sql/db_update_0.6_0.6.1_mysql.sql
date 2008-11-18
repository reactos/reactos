ALTER TABLE {PREFIX}authors ADD userlevel INT(4) UNSIGNED DEFAULT '0';
UPDATE {PREFIX}authors SET userlevel = 255 WHERE authorid = 1;
UPDATE {PREFIX}authors SET userlevel = 1 WHERE authorid > 1;

ALTER TABLE {PREFIX}config  ADD authorid  INT UNSIGNED NOT NULL DEFAULT '0';
CREATE INDEX configauthorid_idx ON {PREFIX}config (authorid);

ALTER TABLE {PREFIX}config  DROP PRIMARY KEY;
ALTER TABLE {PREFIX}config  ADD PRIMARY KEY (name, authorid);

ALTER TABLE {PREFIX}plugins  ADD authorid  INT UNSIGNED DEFAULT '0';
CREATE INDEX pluginauthorid_idx ON {PREFIX}plugins (authorid);

ALTER TABLE {PREFIX}images  ADD authorid  INT UNSIGNED DEFAULT '0';
CREATE INDEX imagesauthorid_idx ON {PREFIX}images (authorid);
