DROP INDEX IF EXISTS ipb_address;
CREATE UNIQUE INDEX ipb_address_unique ON ipblocks (ipb_address,ipb_user,ipb_auto,ipb_anon_only);
