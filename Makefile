#

APPS=test1pub
LIBS=pubnub/pubnub_sync.a

CONF=$(HOME)/.pubnub.conf

CFLAGS=-I. -Wall -O3 -pthread
LDLIBS=-lssl -lcrypto

love:	all

all:	$(APPS)

clean:
	rm -f $(APPS) libsdone
	$(MAKE) -C pubnub -f posix.mk clean

$(APPS):	$(LIBS) private.h

private.h:	$(CONF) Makefile
	bash -c '. "$(CONF)" && for a in $${!PUBNUB_*}; do printf "#define %q \"%q\"\\n" "$$a" "$${!a}"; done > "$@"'

$(LIBS):	.libsdone.tmp
	$(MAKE) -C pubnub -f posix.mk
	touch '$@'

.libsdone.tmp:
	$(MAKE) -C pubnub -f posix.mk
	touch '$@'

