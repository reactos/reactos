CREATE OR REPLACE FUNCTION ts2_page_title()
RETURNS TRIGGER
LANGUAGE plpgsql AS
$mw$
BEGIN
IF TG_OP = 'INSERT' THEN
  NEW.titlevector = to_tsvector('default',REPLACE(NEW.page_title,'/',' '));
ELSIF NEW.page_title != OLD.page_title THEN
  NEW.titlevector := to_tsvector('default',REPLACE(NEW.page_title,'/',' '));
END IF;
RETURN NEW;
END;
$mw$;
