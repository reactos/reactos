ALTER TABLE revision DROP CONSTRAINT revision_rev_user_fkey;
ALTER TABLE revision ADD CONSTRAINT revision_rev_user_fkey
  FOREIGN KEY (rev_user) REFERENCES mwuser(user_id) ON DELETE RESTRICT;

