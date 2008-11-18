ALTER TABLE {PREFIX}entries ADD COLUMN moderate_comments BOOLEAN;
ALTER TABLE {PREFIX}comments ADD COLUMN status varchar(50);
UPDATE {PREFIX}comments SET status = 'approved';
UPDATE {PREFIX}entries SET moderate_comments = false;
ALTER TABLE {PREFIX}entries ALTER COLUMN moderate_comments SET NOT NULL;
ALTER TABLE {PREFIX}comments ALTER COLUMN status SET NOT NULL;

