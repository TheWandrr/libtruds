CC = g++
CFLAGS = -Wall -fvisibility=hidden
LDFLAGS = -lpthread -L./ -ltruds
OUTPUT_FILE = libtruds.a
INSTALL_DIR = /usr/local/lib/truds
CLEAN_EXT = o a d
CLEAN_FILES = test
SOURCES = truds.cpp transit.cpp

.PHONY: all
all: $(OUTPUT_FILE)

test: test.cpp $(OUTPUT_FILE)
	$(CC) test.cpp -o $@ $(LDFLAGS)

$(OUTPUT_FILE): $(SOURCES:.cpp=.o)
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
