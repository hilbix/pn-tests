#

APPS=test1pub
LIBS=pubnub/pubnub_sync.a

CONF=$(HOME)/.pubnub.conf

CFLAGS=-I. -Wall -O3 -pthread
LDLIBS=-lssl -lcrypto

.PHONY:	love
love:	all

.PHONY:	all
all:	$(APPS)

.PHONY:	clean
clean:
	rm -f $(APPS) libsdone
	$(MAKE) -C pubnub -f posix.mk clean

$(APPS):	$(LIBS) private.h

private.h:	$(CONF) Makefile
	bash -c '. "$(CONF)" && for a in $${!PUBNUB_*}; do printf "#define %q \"%q\"\\n" "$$a" "$${!a}"; done > "$@"'

$(LIBS):	pubnub .libsdone.tmp
	$(MAKE) -C pubnub -f posix.mk
	touch '$@'

.libsdone.tmp:
	$(MAKE) -C pubnub -f posix.mk
	touch '$@'

pubnub:
	git submodule update --init

.PHONY:	test
test:
	./test1pub

