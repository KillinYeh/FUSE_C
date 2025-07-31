#ifndef PNODE_H
#define PNODE_H

typedef struct pnode pnode_t;
typedef struct pnode_list pnode_list;
typedef struct HashNode HashNode_t;
typedef struct HashMap HashMap;
struct pnode
{
        __ino_t id;
        char name[256];
        pnode_t* parent_pnode;
        mode_t mode;
        uid_t uid;
        gid_t gid;
        time_t atime,mtime,ctime;
        size_t size;
        char *content;        // for file
        pnode_list* child_head;  // store child pnode with max lenth=256 ;
        int child_count ;
};

//use to track child_list of pnode
struct pnode_list
{
        pnode_t* node;
        pnode_list * next;
};


//use to track pnode of Hashma
struct HashNode
{

        pnode_t* node; // node->id is key
        HashNode_t *next ;

};

struct HashMap
{

        int size;
        HashNode_t** table;

};

#endif
