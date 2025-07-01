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
    	struct pnode* children[256];  // store child pnode with max lenth=256 ;
    	int child_count ;
} pnode_t;

int pnode_id = -1;
pnode_t pnode_table[256];

static int is_root(const char* path)
{
	if( strcmp( path , "/" ) == 0) return 1;
	return 0;
}


pnode_t * find_pnode_by_path(const char* path)
{
    	if (is_root(path)) return &pnode_table[0];
    	const char* last_slash = strrchr(path, '/');
    	if (last_slash == NULL || *(last_slash + 1) == '\0') {
       		// path is invalid or ends with "/"
        	return NULL;
    	}	

    	const char* file_name = last_slash + 1;
	for(int idx=0; idx<=pnode_id ; idx++)
	{
		if( strcmp(pnode_table[idx].name , file_name) == 0)
		{
			printf("file_name : %s find in pnode_id = %d\n", file_name ,idx);
			return &pnode_table[idx];
		}
	}
	return NULL;
}

pnode_t* find_parent_pnode_by_path(const char* path)
{
	char copy_path[256];
	strncpy( copy_path, path , sizeof(copy_path) - 1 );
	copy_path[ sizeof(copy_path) -1 ] = '\0';


	char* parent_path = strrchr(copy_path,'/');
	
	if(parent_path == copy_path) return &pnode_table[0]; //parent is root

	*parent_path = '\0'; //parent is not a root


	pnode_t* parent_node = find_pnode_by_path(parent_path);

	if(parent_node == NULL) return NULL;
	return parent_node;
}

static int pcyfs_getattr(const char* path , struct stat* stbuf)
{
	pnode_t* cur = find_pnode_by_path(path);
	if(cur==NULL){
		printf("can't not find the path = %s in directory \n",path);
		return -ENOENT;
	}
        printf("find path with pnode_id = %d \n",cur->id);

	memset(stbuf,0,sizeof( struct stat ));
	stbuf->st_uid = cur->uid;
	stbuf->st_gid = cur->gid;;
	stbuf->st_atime = cur->atime;
	stbuf->st_mtime = cur->mtime;
	stbuf->st_ino = cur->id;
	if(cur->mode == S_IFDIR) //directory
	{
		printf("this path is directory %s\n",path);
		stbuf->st_mode = (cur -> mode) | 0x0755 ;
		stbuf->st_nlink=2;
	}
	else if ( cur->mode == S_IFREG) //file
	{
		printf("this path is reg_file %s\n",path);
		stbuf->st_nlink=1;
		stbuf->st_mode = (cur -> mode) | 0x0666;
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
	pnode_t* cur = find_pnode_by_path(path);
	if( cur == NULL)
	{
		printf("no such file / directory with path %s \n",path);
		return -ENOENT;
	}
	for(int i=1; i<= cur->child_count;i++)
	{
		struct stat st;	
		memset(&st, 0, sizeof(stat));
    		st.st_ino = cur->children[i]->id;
    		st.st_mode = (cur->children[i]->mode) | 0x0755;
		st.st_uid = cur->children[i]->uid;	
		st.st_gid = cur->children[i]->gid;
		st.st_atime = cur->children[i]->atime;
		st.st_mtime = cur->children[i]->mtime;
		st.st_ctime = cur->children[i]->ctime;
		st.st_nlink = 2;
		st.st_size = cur->children[i]->size;
    		filler(buffer, cur->children[i]->name, &st, 0);	
	}

	return 0;	
}

static int pcyfs_open(const char *path, struct fuse_file_info *fi)
{


	return 0;
}
static int pcyfs_mkdir(const char *path, mode_t mode)
{ 
	pnode_t* parent_pnode = find_parent_pnode_by_path(path);
	if(parent_pnode == NULL) return -ENOENT;	
	pnode_id++ ;
	pnode_t *dir = &pnode_table[pnode_id];
	pnode_t tmp = {
		.id = pnode_id,
		.parent_id = parent_pnode->id,
		.mode = mode ,
		.uid = getuid(),
		.gid = getgid(),
		.atime = time(NULL),
		.mtime = time(NULL),
		.ctime = time(NULL),
		.size = 1024,
		.content = NULL,
		.child_count = 0
	}; 
	char* name = strrchr(path,'/') + 1;
	*dir = tmp;
	strcpy(dir->name,name);
	printf("create directory file name = %s ",dir->name);
	parent_pnode->child_count ++ ;	
	parent_pnode->children[parent_pnode->child_count] = dir;
	pnode_table[pnode_id] = *dir;
	return 0;
}

static struct fuse_operations operations={
	.getattr = pcyfs_getattr,
	.readdir = pcyfs_readdir,
	.mkdir = pcyfs_mkdir
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
		.mode = S_IFDIR ,
		.uid = getuid(),
		.gid = getgid(),
		.atime = time(NULL),
		.mtime = time(NULL),
		.ctime = time(NULL),
		.size = 1024,
		.content = NULL,
		.child_count = 0
	};
	pnode_table[pnode_id] = root;
	printf("root pnode_id = %d",root.id);

	return fuse_main(argc,argv,&operations,NULL);
}
