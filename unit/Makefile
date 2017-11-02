TESTS = $(patsubst %.c,%,$(wildcard *.c))
OBJECTS = $(patsubst %.c,%.o,$(wildcard *.c))
LDFLAGS += $(shell pkg-config --libs r_core) -lm
CFLAGS += $(shell pkg-config --cflags r_core) -g -DUNIT_TEST

all: $(OBJECTS) $(TESTS)

%.o : %.c
	$(CC) $(CFLAGS) $< -c -o $@

% : %.o
	$(CC) $(LDFLAGS) $< -o $@

run:
	r=0 ; for a in $(TESTS) ; do ./$$a || r=1; done ; exit $r

clean:
	rm -f $(TESTS) $(OBJECTS)

.PHONY: all
