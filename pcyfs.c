#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>



static struct fuse_operations perations={
};

int main(int argc,char *argv[])
{
	printf("pcyfs mount at %s",argv[2]);
	return fuse_main(argc,argv,&operations);
}
