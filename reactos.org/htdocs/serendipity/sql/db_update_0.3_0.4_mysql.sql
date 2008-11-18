ALTER TABLE {PREFIX}entries ADD COLUMN allow_comments ENUM('true', 'false') NOT NULL DEFAULT 'true' AFTER isdraft;
