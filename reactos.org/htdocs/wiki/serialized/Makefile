
MESSAGE_SOURCES=$(wildcard ../languages/messages/Messages*.php)
MESSAGE_TARGETS=$(patsubst ../languages/messages/Messages%.php, Messages%.ser, $(MESSAGE_SOURCES))
SPECIAL_TARGETS=Utf8Case.ser
ALL_TARGETS=$(MESSAGE_TARGETS) $(SPECIAL_TARGETS)
DIST_TARGETS=$(SPECIAL_TARGETS) \
	     MessagesDe.ser \
	     MessagesEn.ser \
	     MessagesFr.ser \
	     MessagesJa.ser \
	     MessagesNl.ser \
	     MessagesPl.ser \
	     MessagesSv.ser

.PHONY: all dist clean

all: $(ALL_TARGETS)

dist: $(DIST_TARGETS)

clean:
	rm -f $(ALL_TARGETS)

Utf8Case.ser : ../includes/normal/Utf8Case.php
	php serialize.php -o $@ $<

Messages%.ser : ../languages/messages/Messages%.php ../languages/messages/MessagesEn.php
	php serialize-localisation.php -o $@ $<
