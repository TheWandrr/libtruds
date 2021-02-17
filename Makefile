CC = gcc
CFLAGS = -Wall -fvisibility=hidden
LDFLAGS = -L./ -ltruds
OUTPUT_FILE = libtruds.a
INSTALL_DIR = /usr/local/lib/truds
CLEAN_EXT = o a d
CLEAN_FILES = test
SOURCES = truds.c # Add other source files here

.PHONY: all
all: $(OUTPUT_FILE)

test: test.c $(OUTPUT_FILE)
	$(CC) test.c -o $@ $(LDFLAGS)

$(OUTPUT_FILE): $(SOURCES:.c=.o)
	ar r $@ $^
	ranlib $@

$(SOURCES:.c=.o): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: install
install:
	mkdir -p $(INSTALL_DIR)
	cp -p $(OUTPUT_FILE) $(INSTALL_DIR)

.PHONY: clean
clean:
	for file in $(CLEAN_EXT); do rm -f *.$$file; done
	rm $(CLEAN_FILES)
