ALTER TABLE {PREFIX}entries ADD COLUMN trackbacks INT(4) DEFAULT 0 AFTER `comments`;

ALTER TABLE {PREFIX}entries ADD COLUMN isdraft ENUM('true', 'false') NOT NULL DEFAULT 'false' AFTER `categoryid`;

ALTER TABLE {PREFIX}comments ADD COLUMN subscribed ENUM('true', 'false') NOT NULL DEFAULT 'false' AFTER `type`;

INSERT INTO {PREFIX}config (name, value) VALUES ('allowSubscriptions', 'false');

INSERT INTO {PREFIX}config (name, value) VALUES ('template', 'default');

INSERT INTO {PREFIX}config (name, value) VALUES ('embed', 'false');

INSERT INTO {PREFIX}config (name, value) VALUES ('indexFile', 'index.php');

INSERT INTO {PREFIX}config (name, value) VALUES ('blockReferer', ';');

INSERT INTO {PREFIX}config (name, value) VALUES ('XHTML11', 'false'); 
