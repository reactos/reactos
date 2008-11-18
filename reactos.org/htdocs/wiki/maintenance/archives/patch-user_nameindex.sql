--
-- Change the index on user_name to a unique index to prevent
-- duplicate registrations from creeping in.
--
-- Run maintenance/userDupes.php or through the updater first
-- to clean up any prior duplicate accounts.
--
-- Added 2005-06-05
--

     ALTER TABLE /*$wgDBprefix*/user
      DROP INDEX user_name,
ADD UNIQUE INDEX user_name(user_name);
