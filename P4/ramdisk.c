#define FUSE_USE_VERSION 29

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define FILETYPE 1
#define FOLDERTYPE 2


typedef struct ramdiskNode{
	int type;
	int status;
	int childCount;
	char nodeName[256];
	char *fileMeta;
	struct ramdiskNode *parentNode;
	struct ramdiskNode *childList;
	struct ramdiskNode *nextNode;
	struct stat *nodeStat;
}Node;


char tmpRtrnName[256];
long freeMemory;
Node *rootNode, *tmpRtrnNode;


static void populateNodeParams(Node **newNode){
	time_t tmpTime;
	time(&tmpTime);
	(*newNode)->nodeStat->st_size = 0;
	(*newNode)->childList = NULL;
	(*newNode)->nextNode = NULL;
	(*newNode)->nodeStat->st_atime = tmpTime;
	(*newNode)->nodeStat->st_mtime = tmpTime;
	(*newNode)->nodeStat->st_ctime = tmpTime;
	(*newNode)->nodeStat->st_uid = getuid();
	(*newNode)->nodeStat->st_gid = getgid();
	(*newNode)->fileMeta = NULL;
	(*newNode)->childCount=0;

}

int inspectPath(const char *path){
	if(strcmp(path, "/") == 0){
		tmpRtrnNode = rootNode;
		return 0;
	}
	char tmpPath[512];
	strcpy(tmpPath, path);
	char *token = strtok(tmpPath, "/");
	Node *tmpNode  = rootNode;
	Node *temp = NULL;
	int isPresent;
	while(token != NULL) {
		isPresent = 0;
		temp = tmpNode->childList;
		while(temp != NULL) {
			if(strcmp(temp->nodeName, token) == 0 ){
				isPresent = 1;
				break;
			}
			temp = temp->nextNode;
		}
		token = strtok(NULL, "/");
		if(isPresent == 1) {
			if( token == NULL ){
				tmpRtrnNode = temp;
				return 0;
			}
		}
		else {
			if (token)
				return -1;
			return 1;
		}
		tmpNode = temp;
	}
	return -1;
}

Node *obtainNode(const char *path){
	if(strcmp(path, "/") == 0)
		return rootNode;
	char tmpPath[512];
	strcpy(tmpPath, path);
	char *token = strtok(tmpPath, "/");

	Node *tmpNode  = rootNode;
	Node *temp = NULL;
	int isPresent = 0;
	while(token != NULL){
		isPresent = 0;
		temp = tmpNode->childList;
		while(temp != NULL) {
			if(strcmp(temp->nodeName, token) == 0 ){
				isPresent = 1;
				break;
			}
			temp = temp->nextNode;
		}
		if(isPresent == 1) {
			token = strtok(NULL, "/");
			if( token == NULL )
				return temp;
		}
		else {
			strcpy(tmpRtrnName, token);
			return tmpNode;
		}
		tmpNode = temp;
	}
	return NULL;
}

int createNode(Node **newNode)
{
	(*newNode) = (Node *)malloc(sizeof(Node));
	if((*newNode) == NULL)
		return -1;
	(*newNode)->nodeStat = (struct stat *)malloc(sizeof(struct stat));
	return 0;
}

void pasteFolder(Node **parentNode, Node **newNode)
{
	if((*parentNode)->childList == NULL) {
		(*parentNode)->childList = (*newNode);
	}
	else{
		Node *tempNode;
		tempNode = (*parentNode)->childList;
		while(tempNode->nextNode != NULL){
			tempNode = tempNode->nextNode;
		}
        tempNode->nextNode = (*newNode);
	}
	(*parentNode)->childCount++;
}

static int openFile(const char *path, struct fuse_file_info *fi)
{
	if(inspectPath(path) == 0)
		return 0;
	return -ENOENT;
}

static int readFile(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	Node *tmpNode;
	size_t len;
	tmpNode = obtainNode(path);
	if(tmpNode->type == FOLDERTYPE)
		return -EISDIR;	
	
	len = tmpNode->nodeStat->st_size;
	if(offset < len) {
		if(offset + size > len)		
			size = len - offset;		
		memcpy(buf, tmpNode->fileMeta + offset, size);
	} else	
		size = 0;
		
	if(size > 0) {
		time_t tmpTime;
		time(&tmpTime);
		tmpNode->nodeStat->st_atime = tmpTime;
	}
	return size;	
}

static int writeFile(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	Node *tmpNode;
	tmpNode = obtainNode(path);	
	if(size > freeMemory)
		return -ENOSPC;
	
	if( size > 0) {
		size_t len;
		len = tmpNode->nodeStat->st_size;
		time_t tmpTime;
		time(&tmpTime);
		if(len == 0) {
			tmpNode->fileMeta = (char *)malloc(sizeof(char) * size);
			memcpy(tmpNode->fileMeta , buf, size);
			tmpNode->nodeStat->st_size = size;
			tmpNode->nodeStat->st_mtime = tmpTime;
			freeMemory -= size;
		}		
		else {
			if(offset > len)
				offset = len;
			char *tmpFileMeta = (char *)realloc(tmpNode->fileMeta, sizeof(char) * (offset+size));
			if(tmpFileMeta == NULL)
				return -ENOSPC;
			else{
				tmpNode->fileMeta = tmpFileMeta;
				memcpy(tmpNode->fileMeta + offset, buf, size);
				tmpNode->nodeStat->st_size = offset + size;
				tmpNode->nodeStat->st_mtime = tmpTime;
				freeMemory += len;
				freeMemory -= (offset + size);
			}
		}
	}
	return size;
}

static int createFile(const char *path, mode_t mode, struct fuse_file_info *fi){
	Node *parentNode = obtainNode(path);
	Node *newNode;

	if(createNode(&newNode) == -1)
		return -ENOSPC;
	freeMemory -= (sizeof(Node) + sizeof(stat));
	strcpy(newNode->nodeName, tmpRtrnName);
	newNode->type = FILETYPE;
	newNode->parentNode = parentNode;
	newNode->nodeStat->st_mode = S_IFREG | mode;
	newNode->nodeStat->st_nlink = 1;
	populateNodeParams(&newNode);
	pasteFolder(&parentNode, &newNode);
	return 0;
}

static int createFolder(const char *path, mode_t mode)
{
	Node *parentNode = obtainNode(path);
	Node *newFolder;
	if(createNode(&newFolder) == -1)
		return -ENOSPC;

	freeMemory -= (sizeof(Node) + sizeof(struct stat));
	strcpy(newFolder->nodeName, tmpRtrnName);
	newFolder->parentNode = parentNode;
	populateNodeParams(&newFolder);
	newFolder->type = FOLDERTYPE;
	newFolder->nodeStat->st_size = 4096;
	newFolder->nodeStat->st_nlink = 2;
	newFolder->nodeStat->st_mode = S_IFDIR | 0755;
	pasteFolder(&parentNode, &newFolder);
	parentNode->nodeStat->st_nlink = parentNode->nodeStat->st_nlink + 1;
	return 0;
}

void removeNode(Node **parentNode, Node **removedNode){
	if((*parentNode)->childList == (*removedNode)){
		if((*removedNode)->nextNode != NULL)
			(*parentNode)->childList = (*removedNode)->nextNode;
		else
			(*parentNode)->childList = NULL;
	}		  
	else{
		Node *tmpNode;
		tmpNode = (*parentNode)->childList;
		while(tmpNode != NULL){
			if(tmpNode->nextNode == (*removedNode)){
				tmpNode->nextNode = (*removedNode)->nextNode;
				break;
			}
			tmpNode = tmpNode->nextNode;
		}
		(*parentNode)->childCount--;
	}
}

//Below method implemented for removing a file
static int removeFile(const char *path)
{
	if(inspectPath(path) == 0){
		Node *tmpNode, *parentNode;
		tmpNode = obtainNode(path);
		parentNode = tmpNode->parentNode;
		removeNode(&parentNode, &tmpNode);
		if(tmpNode->nodeStat->st_size != 0){
			freeMemory += tmpNode->nodeStat->st_size;
			free(tmpNode->fileMeta);
		}
		free(tmpNode->nodeStat);
		free(tmpNode);
		freeMemory += (sizeof(Node) + sizeof(struct stat));
		return 0;
	}
	return -ENOENT;
}

static int removeFolder(const char *path){
	if(inspectPath(path) == 0){
		Node *parentNode;
		parentNode = tmpRtrnNode->parentNode;
		removeNode(&parentNode, &tmpRtrnNode);
		free(tmpRtrnNode->nodeStat);
		free(tmpRtrnNode);
		freeMemory += (sizeof(Node) + sizeof(struct stat));
		parentNode->nodeStat->st_nlink = parentNode->nodeStat->st_nlink-1;
		return 0;
	}
	return -ENOENT;
}

static int readFolder(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
	if(inspectPath(path) != -1){
		Node *folderNode, *tmpNode;
		folderNode = obtainNode(path);
		time_t tmpTime;
		time(&tmpTime);
		folderNode->nodeStat->st_atime = tmpTime;
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		tmpNode = folderNode->childList;
		while(tmpNode != NULL){
			filler(buf, tmpNode->nodeName, NULL, 0);
			tmpNode = tmpNode->nextNode;
		}
		return 0;
	}
	return -ENOENT;
}

static int getFileAttr(const char *path, struct stat *stbuf){
	if(inspectPath(path) == 0){
		stbuf->st_size = tmpRtrnNode->nodeStat->st_size;
		stbuf->st_nlink = tmpRtrnNode->nodeStat->st_nlink;
		stbuf->st_mode  = tmpRtrnNode->nodeStat->st_mode;
		stbuf->st_uid   = tmpRtrnNode->nodeStat->st_uid;
		stbuf->st_gid   = tmpRtrnNode->nodeStat->st_gid;
		stbuf->st_ctime = tmpRtrnNode->nodeStat->st_ctime;
		stbuf->st_mtime = tmpRtrnNode->nodeStat->st_mtime;
		stbuf->st_atime = tmpRtrnNode->nodeStat->st_atime;
		return 0;
	}
	return -ENOENT;
}

// Below function to supress "setting times of ‘one’: Function not implemented" warning
static int modifyAccessTime(const char *path, struct utimbuf *ubuf){
	return 0;
}

static struct fuse_operations my_fuse_operations = {
	//Prof List of Sys calls
	.open		= openFile,
	.read		= readFile,
	.write		= writeFile,
	.create		= createFile,
	.mkdir		= createFolder,
	.unlink		= removeFile,
	.rmdir		= removeFolder,
	.readdir	= readFolder,
	.getattr	= getFileAttr,
	.utimens	= modifyAccessTime
};

int main(int argc, char *argv[])
{
	if( argc != 3){
		printf("Invalid number of arguments. \n Usage:ramdisk <mount_point> <size> \n");
		return -1;
	}
	int fuse_argc = argc-1;
	freeMemory = (long) atoi(argv[2]);
	freeMemory = freeMemory * 1024 * 1024;
	rootNode = (Node *)malloc(sizeof(Node));
	rootNode->nodeStat = (struct stat *)malloc(sizeof(struct stat));
	freeMemory -= (sizeof(Node) + sizeof(stat));
	strcpy(rootNode->nodeName, "/");
	rootNode->type = FOLDERTYPE;
	rootNode->parentNode = NULL;
	populateNodeParams(&rootNode);
	rootNode->nodeStat->st_mode = S_IFDIR | 0755;
	rootNode->nodeStat->st_nlink = 2;
	rootNode->nodeStat->st_uid = 0;
	rootNode->nodeStat->st_gid = 0;
	fuse_main(fuse_argc, argv, &my_fuse_operations, NULL);
	return 0;
}
