CFLAGS = -g -Wall -pedantic
BUILD = extension src

all: $(BUILD)

extension:
	cd extension && $(MAKE)

src:
	cd src && $(MAKE)

clean:
	cd src && $(MAKE) clean
	cd extension && $(MAKE) clean

.PHONY: extension src
