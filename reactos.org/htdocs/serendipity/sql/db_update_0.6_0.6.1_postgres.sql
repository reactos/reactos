
ALTER TABLE {PREFIX}authors ADD COLUMN userlevel SMALLINT;
ALTER TABLE {PREFIX}authors ALTER COLUMN userlevel SET DEFAULT '0';
UPDATE {PREFIX}authors SET userlevel = 255 WHERE authorid = 1;
UPDATE {PREFIX}authors SET userlevel = 1 WHERE authorid > 1;

ALTER TABLE {PREFIX}config  ADD COLUMN authorid  INT;
UPDATE {PREFIX}config SET authorid=0;
ALTER TABLE {PREFIX}config  ALTER COLUMN authorid SET NOT NULL;
ALTER TABLE {PREFIX}config  ALTER COLUMN authorid SET DEFAULT '0';
CREATE INDEX configauthorid_idx ON {PREFIX}config (authorid);

ALTER TABLE {PREFIX}config  DROP CONSTRAINT {PREFIX}config_pkey;
ALTER TABLE {PREFIX}config  ADD PRIMARY KEY (name, authorid);

ALTER TABLE {PREFIX}plugins  ADD COLUMN authorid  INT;
ALTER TABLE {PREFIX}plugins  ALTER COLUMN authorid SET DEFAULT '0';
CREATE INDEX pluginsauthorid_idx ON {PREFIX}plugins (authorid);

ALTER TABLE {PREFIX}images  ADD COLUMN authorid  INT;
ALTER TABLE {PREFIX}images  ALTER COLUMN authorid SET DEFAULT '0';
CREATE INDEX imagesauthorid_idx ON {PREFIX}images (authorid);
