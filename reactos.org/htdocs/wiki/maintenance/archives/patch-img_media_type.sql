-- media type columns, added for 1.5
-- this alters the scheme for 1.5, img_type is no longer used.

ALTER TABLE /*$wgDBprefix*/image ADD (
  -- Media type as defined by the MEDIATYPE_xxx constants
  img_media_type ENUM("UNKNOWN", "BITMAP", "DRAWING", "AUDIO", "VIDEO", "MULTIMEDIA", "OFFICE", "TEXT", "EXECUTABLE", "ARCHIVE") default NULL,
  
  -- major part of a MIME media type as defined by IANA
  -- see http://www.iana.org/assignments/media-types/
  img_major_mime ENUM("unknown", "application", "audio", "image", "text", "video", "message", "model", "multipart") NOT NULL default "unknown",
  
  -- minor part of a MIME media type as defined by IANA
  -- the minor parts are not required to adher to any standard
  -- but should be consistent throughout the database
  -- see http://www.iana.org/assignments/media-types/
  img_minor_mime varbinary(32) NOT NULL default "unknown"
);
