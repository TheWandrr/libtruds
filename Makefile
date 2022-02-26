CC = gcc
CFLAGS = -Wfatal-errors -Wall -fvisibility=hidden
LDFLAGS = -lpthread
OUTPUT_FILE = libtruds.a
INSTALL_DIR = /usr/local/lib/truds
CLEAN_EXT = o a d
CLEAN_FILES = test
SOURCES = truds.c

.PHONY: all
all: $(OUTPUT_FILE)

test: test.c $(OUTPUT_FILE)
	$(CC) test.c -o $@ -L./ -ltruds

$(OUTPUT_FILE): $(SOURCES:.c=.o)
	ar r $@ $^
	ranlib $@

$(SOURCES:.c=.o): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)

.PHONY: install
install:
	mkdir -p $(INSTALL_DIR)
	cp -p $(OUTPUT_FILE) $(INSTALL_DIR)

.PHONY: clean
clean:
	for file in $(CLEAN_EXT); do rm -f *.$$file; done
	rm $(CLEAN_FILES)
