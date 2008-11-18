-- Log_id field that means one log entry can be referred to with a single number,
-- rather than a dirty great big mess of features.
-- This might be useful for single-log-entry deletion, et cetera.
-- Andrew Garrett, February 2007.

ALTER TABLE /*$wgDBprefix*/logging
	ADD COLUMN log_id int unsigned not null auto_increment,
	ADD PRIMARY KEY log_id (log_id);
