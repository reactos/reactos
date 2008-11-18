ALTER TABLE {PREFIX}authors ADD COLUMN right_publish int2;
ALTER TABLE {PREFIX}authors ALTER COLUMN right_publish SET DEFAULT '1';
UPDATE {PREFIX}authors SET right_publish=1;
