#

APPS=test1pub
SUBS=pubnub
LIBS=pubnub/pubnub_sync.a

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

$(APPS):	$(LIBS)

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

