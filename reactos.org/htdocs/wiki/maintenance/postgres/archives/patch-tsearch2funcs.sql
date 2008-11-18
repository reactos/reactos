-- Should be run on Postgres 8.3 or newer to remove the 'default'

CREATE OR REPLACE FUNCTION ts2_page_title()
RETURNS TRIGGER
LANGUAGE plpgsql AS
$mw$
BEGIN
IF TG_OP = 'INSERT' THEN
  NEW.titlevector = to_tsvector(REPLACE(NEW.page_title,'/',' '));
ELSIF NEW.page_title != OLD.page_title THEN
  NEW.titlevector := to_tsvector(REPLACE(NEW.page_title,'/',' '));
END IF;
RETURN NEW;
END;
$mw$;

CREATE OR REPLACE FUNCTION ts2_page_text()
RETURNS TRIGGER
LANGUAGE plpgsql AS
$mw$
BEGIN
IF TG_OP = 'INSERT' THEN
  NEW.textvector = to_tsvector(NEW.old_text);
ELSIF NEW.old_text != OLD.old_text THEN
  NEW.textvector := to_tsvector(NEW.old_text);
END IF;
RETURN NEW;
END;
$mw$;
