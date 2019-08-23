#

APPS=test1pub test1sub
SUBS=pubnub
LIBS=pubnub/pubnub_sync.a
HEAD=ezpn.h pubnub_config.h pubnub_sync.h

CFLAGS=-I. -Wall -Wno-unused-function -O3 -pthread
LDLIBS=-lssl -lcrypto

.PHONY:	love
love:	all

.PHONY:	all
all:	$(APPS)

.PHONY:	clean
clean:
	rm -f $(APPS) libsdone
	$(MAKE) -C pubnub -f posix.mk clean

$(APPS):	$(LIBS) $(HEAD)

$(LIBS):	$(SUBS) .libsdone.tmp
	$(MAKE) -C pubnub -f posix.mk
	touch '$@'

.libsdone.tmp:
	$(MAKE) -C pubnub -f posix.mk
	touch '$@'

$(SUBS):
	git submodule update --init

.PHONY:	test
test:
	# Set PUBNUB_PUB_KEY/PUBNUB_SUB_KEY first
	./test1pub

