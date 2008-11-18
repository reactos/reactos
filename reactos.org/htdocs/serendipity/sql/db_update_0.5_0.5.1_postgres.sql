CREATE TABLE {PREFIX}images (
  id SERIAL primary key,
  name varchar(255) NOT NULL default '',
  extension varchar(5) NOT NULL default '',
  mime varchar(15) NOT NULL default '',
  size int NOT NULL default '0',
  dimensions_width int NOT NULL default '0',
  dimensions_height int NOT NULL default '0',
  thumbnail_name varchar(255) NOT NULL default '',
  date int NOT NULL default '0'
);

UPDATE {PREFIX}plugins
   SET name = REPLACE(name, 'serendipity_plugin_content_rewrite', 'serendipity_event_contentrewrite')
 WHERE name LIKE '%serendipity_plugin_content_rewrite%';

UPDATE {PREFIX}config
   SET name = REPLACE(name, 'serendipity_plugin_content_rewrite', 'serendipity_event_contentrewrite')
 WHERE name LIKE '%serendipity_plugin_content_rewrite%';

UPDATE {PREFIX}plugins
   SET placement = 'event'
 WHERE name LIKE '%serendipity_event_contentrewrite%';

ALTER TABLE {PREFIX}entries ADD COLUMN last_modified INT;

CREATE INDEX date_idx ON {PREFIX}entries (timestamp);
CREATE INDEX mod_idx ON {PREFIX}entries (last_modified);
UPDATE {PREFIX}entries SET last_modified = timestamp;

INSERT INTO {PREFIX}plugins(name, placement, sort_order) VALUES ('serendipity_event_nl2br:39979424fea674e78399659e67edaf12', 'event', 10);
INSERT INTO {PREFIX}plugins(name, placement, sort_order) VALUES ('serendipity_event_s9ymarkup:fdd8bf23ff500827cf76d7d31957d3c8', 'event', 11);
INSERT INTO {PREFIX}plugins(name, placement, sort_order) VALUES ('serendipity_event_emoticate:e30b433546aa10a632b0d703ebe1aa29', 'event', 12);
INSERT INTO {PREFIX}plugins(name, placement, sort_order) VALUES ('serendipity_event_trackexits:25e6d8eb7d662936abb6454956aaa8ab', 'event', 13);
