COMPILER = gcc
FILESYSTEM_FILES = pcyfs.c

build: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -o pcyfs `pkg-config fuse --cflags --libs`
	echo 'To Mount: ./pcyfs -f [mount point]'

clean:
	
