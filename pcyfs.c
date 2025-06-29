#define FUSE_USE_VERSION 30

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>


typedef struct pnode
{
	int id;
	char name[256];
	int parent_id;
	mode_t mode;
	uid_t uid;
	gid_t gid;
	time_t atime,mtime,ctime;
	size_t size;
    	char *content;        // for file
    	struct inode *children;  // for directory
    	int child_count ;
} pnode_t;

int pnode_id = -1;
pnode_t pnode_table[256];

pnode_t * find_pnode_by_path(const char* path)
{
	for(int idx=0; idx<=pnode_id ; idx++)
	{
		if( strcmp(pnode_table[idx].name , path) == 0)
		{
			printf("path : %s find in pnode_id = %d\n", path,idx);
			return &pnode_table[idx];
		}
	}
	return NULL;
}


static int pcyfs_getattr(const char* path , struct stat* stbuf)
{
	pnode_t* cur = find_pnode_by_path(path);
	if(cur==NULL){
		printf("can't not find the path = %s in directory",path);
		return -ENOENT;
	}
        printf("find path with pnode_id = %d ",cur->id);

	memset(stbuf,0,sizeof( struct stat ));
	stbuf->st_uid = cur->uid;
	stbuf->st_gid = cur->gid;;
	stbuf->st_atime = cur->atime;
	stbuf->st_mtime = cur->mtime;
	int offset = strlen(path) - 1;
	if(cur->mode == (S_IFDIR | 0x0755)) //directory
	{
		printf("this path is directory %s\n",path);
		stbuf->st_mode = cur->mode;
		stbuf->st_nlink=2;
	}
	else if ( cur->mode == (S_IFREG | 0x0666 )) //file
	{
		printf("this path is reg_file %s\n",path);
		stbuf->st_nlink=1;
		stbuf->st_mode = cur->mode;
		stbuf->st_size=1024;
	}
	else{
		return -ENOENT; //no such file or directory
	}
		
	return 0;

}

static int pcyfs_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	filler(buffer, "." , NULL,0 );
	filler(buffer, "..", NULL,0 );
	/*if( strcmp ( path,"/" ) != 0)
	{	
		printf("read directory fail by path = %s\n",path);
		return -ENOENT;
	}
	else{
		filler(buffer,path,NULL,0);
	}*/
	return 0;	
}

static int pcyfs_open(const char *path, struct fuse_file_info *fi)
{


	return 0;
}
static int pcyfs_mkdir(const char *path, mode_t mode)
{
	
	return 0;
}

static struct fuse_operations operations={
	.getattr = pcyfs_getattr,
	.readdir = pcyfs_readdir
	//.open = pcyfs_open
};


int main(int argc,char *argv[])
{
	printf("pcyfs mount at %s\n",argv[4]);
	pnode_id++ ;
	pnode_t root =
	{
		.id = pnode_id,
		.name = "/",
		.parent_id = -1,
		.mode = S_IFDIR | 0x0755,
		.uid = getuid(),
		.gid = getgid(),
		.atime = time(NULL),
		.mtime = time(NULL),
		.ctime = time(NULL),
		.size = 1024,
		.content = NULL,
		.children = NULL,
		.child_count = 0
	};
	pnode_table[pnode_id] = root;
	printf("root pnode_id = %d",&root.id);

	return fuse_main(argc,argv,&operations,NULL);
}
