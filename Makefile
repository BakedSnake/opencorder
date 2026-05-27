PREFIX=/usr/local
INSTALL_DIR=$(PREFIX)/bin
ASSETS_PREFIX=/usr/share
ASSETS_DIR=$(ASSETS_PREFIX)/opencorder

CC = clang
CFLAGS = -Wall $(shell pkg-config --cflags libpipewire-0.3 libspa-0.2)
LDFLAGS = $(shell pkg-config --libs libpipewire-0.3 libspa-0.2)
LIBS := -lm -lsndfile -lraylib
SOURCES = corder.c pipe.c ui.c config.c
ASSETS = assets/*
OBJECTS = $(SOURCES:.c=.o)
TARGET = corder

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) $(LDFLAGS)

install:
	mkdir -p $(ASSETS_DIR)/assets
	install -m 0644 $(ASSETS) $(ASSETS_DIR)/assets
	install -m 0775 $(TARGET) $(INSTALL_DIR)

uninstall:
	rm $(INSTALL_DIR)/$(TARGET)
	rm -rf $(ASSETS_DIR)/$(ASSETS)

clean:
	rm -f $(TARGET) $(OBJECTS)
