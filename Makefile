COMPILER = gcc
FILESYSTEM_FILES = pcyfs.c
.PHONY: all,clean
all: $(FILESYSTEM_FILES)
	$(COMPILER) -g -Wall $(FILESYSTEM_FILES) -o pcyfs `pkg-config fuse --cflags --libs`
	echo 'To Mount: ./pcyfs -o use_ino -f [mount point]'

clean:
	rm ./pcyfs
	
