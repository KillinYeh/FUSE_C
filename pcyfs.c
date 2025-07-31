#define FUSE_USE_VERSION 30

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "pnode.h"
#define MapSize 10

HashMap* map; 
__ino_t pnode_id = 0;




void InitHashMap(HashMap *map)
{
	map->size = MapSize;
	map->table =(HashNode_t **) malloc(MapSize * sizeof(HashNode_t *));
	
	if(map == NULL || map->table == NULL)
	{
		printf("create error ! can't not allocate table \n");
	}

	for(int i=0; i<MapSize ; i++)
	{
		map->table[i] = (HashNode_t * ) malloc( sizeof(HashNode_t ));
		map->table[i]->node = NULL;
		map->table[i]->next = NULL; // dummy head (= table[i] ) 
	}	
}


int calculate_key_by_path(const char* path)
{
	int key = 0;
	while(*path != '\0')
	{
		key += *path;
		path++;
	}
	return key;
}
void put_HashNode(HashMap * map , pnode_t* node)
{
	int key = calculate_key_by_path(node->name);
	key = key % (MapSize);
	
	HashNode_t * head = map->table[key];	
	if(head->next == NULL) // first element
	{

		HashNode_t* new_node =(HashNode_t *) malloc( sizeof(HashNode_t) );
		new_node->node = node; // shallow copy
		new_node->next = NULL;

		head->next = new_node;
	}
	else 
	{
		HashNode_t* pt = head; //traverse linklist from head
		while(pt->next != NULL)
		{
			pt = pt->next;
		}
		HashNode_t* new_node = malloc( sizeof(HashNode_t) );
		new_node->node = node;
		new_node->next = NULL;
		pt->next = new_node;
	}

}
HashNode_t * get_HashNode_by_path(HashMap * map, const char* path)
{
	int key = calculate_key_by_path(path);
	key = key % (MapSize);
	HashNode_t* pt = map->table[key];  // corresponding head of Hashmap  
	while(pt != NULL)
	{
		if (pt->node == NULL)
		{
			pt = pt -> next;
			continue;
		}
		if( strcmp( pt->node->name , path ) == 0 )
		{
			return pt;
		}
		pt = pt->next ;
	}
	return NULL;
}

int remove_HashNode_by_path(HashMap * map, const char* path)
{
	int key = calculate_key_by_path(path);
	key = key % (map->size);

	HashNode_t* head = map->table[key];

	HashNode_t* pt = head;
	HashNode_t* prev = NULL;
	while(pt != NULL)
	{
		if(pt->node == NULL)
		{
			prev = pt;
			pt = pt-> next;
			continue;
		}
		if( strcmp( pt->node->name , path ) == 0 )
		{
			prev->next = pt->next;
			printf(" remove name  = %s from HashMap \n" , pt->node->name) ;
			free(pt);
			return 0;	
		}
		prev = pt;
		pt = pt->next;
	}
	return -1;  //-1 for Fail Remove
} 


void remove_child_from_parent(pnode_t * parent_pnode , pnode_t * child_node)
{
	pnode_list* prev = NULL;
	pnode_list* head = parent_pnode->child_head;
	
	pnode_list* pt = head;
	while(pt != NULL)
	{
		if(pt -> node == NULL )
		{
			prev = pt;
			pt = pt->next;
			continue;
		}
		if(pt->node->id == child_node->id)  //identify node.id to remove specify node 
		{
			prev->next = pt->next;
			printf(" remove child name = %s , from parent name = %s \n" , pt->node->name , parent_pnode->name ) ;
			free(pt->node);  // care double free in another remove func
			free(pt);
			return ;
		}
		prev = pt;
		pt = pt -> next;
	}
}


pnode_t * find_pnode_by_path(HashMap* map ,const char* path)
{
    	if (path == NULL) return NULL; 

	HashNode_t* HashNode = get_HashNode_by_path(map,path);
	if(HashNode != NULL) return HashNode->node;

	return NULL;
}

pnode_t* find_parent_pnode_by_path(HashMap* map , const char* path)
{
	if( strcmp(path , "/" ) == 0 ) return NULL;

	char copy_path[256];
	strncpy( copy_path, path , sizeof(copy_path) - 1 );
	copy_path[ sizeof(copy_path) -1 ] = '\0';


	char* parent_path = strrchr(copy_path,'/');
	
	if(parent_path == copy_path) //parent is root
	{	
		*(parent_path + 1)  = '\0'; //parent is not a root
		printf("parent is root ! print parent_path = %s \n" ,parent_path);
		HashNode_t* HashNode = get_HashNode_by_path(map,parent_path);
		if(HashNode != NULL) return HashNode->node;
	}
	*parent_path = '\0'; //parent is not a root
	parent_path = copy_path; // move pointer to first element to get correct file name	
	printf("print parent_path = %s \n" ,parent_path);
	HashNode_t* HashNode = get_HashNode_by_path(map,parent_path);
	if(HashNode != NULL) return HashNode->node;

	return NULL;

}

static int pcyfs_getattr(const char* path , struct stat* stbuf)
{
	pnode_t* cur = find_pnode_by_path(map,path);
	if( cur == NULL ){
		printf("getattr fail ! can't not find the path = %s in file/directory \n",path);
		return -ENOENT;
	}
        printf("getattr success ! find path = %s with pnode_id = %ld \n",cur->name , cur->id);

	memset(stbuf,0,sizeof( struct stat ));
	stbuf->st_uid = cur->uid;
	stbuf->st_gid = cur->gid;;
	stbuf->st_atime = cur->atime;
	stbuf->st_mtime = cur->mtime;
	stbuf->st_ino = cur->id;
	if(cur->mode == S_IFDIR) //directory
	{
		printf("getattr success ! this path is directory %s\n",path);
		stbuf->st_mode = (cur -> mode) | 0755 ;
		stbuf->st_nlink = 2;
	}
	else if ( cur->mode == S_IFREG ) //file
	{
		printf("getattr success ! this path is reg_file %s\n",path);
		stbuf->st_nlink = 1;
		stbuf->st_mode = (cur -> mode) | 0666;
		stbuf->st_size = 4096;
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
	pnode_t* cur = find_pnode_by_path(map,path);
	if( cur == NULL)
	{
		printf("readdir fail ! no such file / directory with path %s \n",path);
		return -ENOENT;
	}
	pnode_list* head = cur->child_head;


	pnode_list* pt = head;
	while(pt != NULL)
	{
		if(pt->node == NULL)
		{
			pt = pt -> next;
			continue;
		}
		struct stat st;	
		memset(&st, 0, sizeof(stat));
    		st.st_ino = pt->node->id; // in filler function, FUSE only reference st_ino & st_mode.
    		st.st_mode = ( pt->node->mode );

		char* last_name = strrchr(pt->node->name , '/');  // warning ! cat't not put the path(any file name with "/") into filler function 
		last_name ++; // remove "/"
		printf("readdir suucess ! name = %s , st_ino = %ld , st_mode = %d \n",last_name , st.st_ino , st.st_mode);
    		filler(buffer, last_name, &st , 0);	
		pt = pt -> next;
	}

	return 0;	
}

static int pcyfs_open(const char *path, struct fuse_file_info *fi)
{
	pnode_t* node = find_pnode_by_path(map,path);

	printf("enter open success \n"); 
	if( node == NULL) return -ENOENT;

	if( node->mode == S_IFDIR )return -EISDIR;

	fi->fh =(uint64_t) node; //save pnode info into fh

	pnode_t* test = (pnode_t*) fi->fh;	
	
	printf("open success ! file name = %s , with file id = %ld \n", test->name , test->id ); 
	return 0;
}

static int pcyfs_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
{
	pnode_t* file = (pnode_t*) info->fh;
	
	if(file == NULL) return -ENOENT;
 
	printf("write function !! file name = %s , write size = %ld \n",file->name , size);	
	memcpy( file->content+offset , buffer , size );
		
	return size;
}

static int pcyfs_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{

	pnode_t* file = (pnode_t*) fi->fh ;
	if(file == NULL) return -ENOENT;

	size_t len = strlen(file->content);
	printf(" read! file = %s , old size = %ld   size = %ld \nn" , file->name, len , size);
	
	memcpy( buffer , file-> content+offset , size );
		
	return strlen(file->content) - offset;
}




static int pcyfs_mkdir(const char *path, mode_t mode)
{ 
	pnode_t* parent_pnode = find_parent_pnode_by_path(map,path);
	if(parent_pnode == NULL) return -ENOENT;	
	pnode_t* dir = (pnode_t *) malloc(sizeof(pnode_t));
	dir->id = pnode_id;
	dir->parent_pnode  = parent_pnode;
	dir->mode = S_IFDIR ;
	dir->uid = getuid();
	dir->gid = getgid();
	dir->atime = time(NULL);
	dir->mtime = time(NULL);
	dir->ctime = time(NULL);
	dir->content = NULL;
	dir->child_count = 0; 
	strcpy(dir->name,path);
	pnode_id++ ;
	dir->child_head = (pnode_list*) malloc(sizeof(pnode_list)); //dummy head
	dir->child_head->node = NULL;
	dir->child_head->next = NULL;

	put_HashNode( map , dir );


	printf("Mkdir ! create directory  name = %s \n ",dir->name);
	parent_pnode->child_count ++ ;
	pnode_list* head = parent_pnode->child_head;
	
	printf("parent path = %s \n" , parent_pnode->name );
	pnode_list* pt = head;
	while(pt->next != NULL)
	{
		pt = pt->next;
	}
	pnode_list* new_child = malloc(sizeof(pnode_list));
	new_child->node = dir;
	new_child->next = NULL;
	pt->next = new_child;
	return 0;
}

static int pcyfs_unlink(const char* path)
{
	pnode_t* parent_pnode = find_parent_pnode_by_path(map,path);
	pnode_t* file = find_pnode_by_path(map,path);

	
	if(file == NULL) return -ENOENT;
	
	remove_HashNode_by_path(map , file->name);
	remove_child_from_parent(parent_pnode,file);

	return 0;
}



static int pcyfs_mknod(const char* path,mode_t mode , dev_t rdev)
{
	
	pnode_t* parent_pnode = find_parent_pnode_by_path( map, path );
	if(parent_pnode == NULL) return -ENOENT;	
	pnode_t * file = (pnode_t *)  malloc(sizeof(pnode_t));

	file->id = pnode_id;
	file->parent_pnode = parent_pnode;
	file->mode = S_IFREG ;
	file->uid = getuid();
	file->gid = getgid();
	file->atime = time(NULL);
	file->mtime = time(NULL);
	file->ctime = time(NULL);
	file->child_count = 0;
        file->size = 4096;	
	file->content = (char *) malloc(file->size);
	strcpy(file->name,path);
	file->child_head = (pnode_list*) malloc(sizeof(pnode_list));
	file->child_head->node = NULL;
	file->child_head->next = NULL;  //dummy head

	put_HashNode(map , file);
	pnode_id++ ;

	parent_pnode->child_count ++ ;	
	pnode_list* pt = parent_pnode->child_head;
	while(pt->next != NULL)
	{
		pt = pt->next;
	}
	pnode_list* new_child = malloc(sizeof(pnode_list));
	new_child->node = file;
	new_child->next = NULL;
	pt->next = new_child;
	printf("Mknod success ! create file name = %s with pnode id = %ld \n" , file->name , file->id );
	return 0;
}



static struct fuse_operations operations={
	.getattr = pcyfs_getattr,
	.readdir = pcyfs_readdir,
	.mkdir = pcyfs_mkdir,
	.mknod = pcyfs_mknod,
	.read = pcyfs_read,
	.write = pcyfs_write,
	.open = pcyfs_open,
	.unlink = pcyfs_unlink
};


int main(int argc,char *argv[])
{
	printf("pcyfs mount at %s\n",argv[4]);
	pnode_t* root = (pnode_t *) malloc(sizeof(pnode_t));
		
	root->id = pnode_id,
	root->parent_pnode = NULL,
	root->mode = S_IFDIR ,
	root->uid = getuid(),
	root->gid = getgid(),
	root->atime = time(NULL),
	root->mtime = time(NULL),
	root->ctime = time(NULL),
	root->content = NULL,
	root->child_count = 0;
	strcpy(root->name , "/");
	root->child_head =(pnode_list*) malloc(sizeof( pnode_list));

	root->child_head -> node = NULL;
	root->child_head -> next = NULL; //dummy head for easy implement remove operation
	map = malloc(sizeof(HashMap));
	InitHashMap(map);
	put_HashNode(map,root);
	pnode_id ++ ;
	printf("root pnode_id = %ld",root->id);

	return fuse_main(argc,argv,&operations,NULL);
}
