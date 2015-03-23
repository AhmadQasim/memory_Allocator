#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "mem.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


typedef struct {
	short size;
	short magic;
	short prev;
	bool available;
	short bprev;
	short right;
	short left;
	short side;
}__attribute__((packed))fL;


fL *newSpace(short sizeR);
void removeBnode (fL *node);
void insertBnode (fL *newN);
fL *addLinked(fL *node, short size);
void initNode(void *node);

void *mm = NULL;
short binaryH = -1, linkedH = -1, linkedT = -1;
int region = 0, sizeT = 0;


int Mem_Init(int sizeOfRegion){
	sizeT = sizeOfRegion;
	if (mm==NULL){
		int fd = open("/dev/zero", O_RDWR);
		mm = mmap(NULL, (sizeOfRegion), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
		initNode(mm);
		initNode(mm+sizeOfRegion);
		
		if (mm == MAP_FAILED) { 
			perror("mmap");
			return -1;
		}
		close(fd);
		newSpace(sizeOfRegion);
		return 0; 
	}
	else 
		return -1;
}

void *Mem_Alloc(int size){
	if (mm==NULL || size <= 0 || size > sizeT)
		return NULL;
	void *newvN = newSpace(size+12);
	if (newvN != NULL)
		return (newvN+12);
	else
		return NULL;
}
fL *newSpace(short sizeR) {
	if (linkedH == -1){
		fL *node = mm;
		node->size = sizeR-32;
		node->magic = 0;
		node->available = 1;
		node->right = -1;
		node->left = -1;
		node->side = -1;
		node->prev = -1;
		node->bprev = -1;
		binaryH = 0;
		linkedH = 0;
		linkedT = 0;
		return NULL;
	}
	fL *node = (mm + binaryH);
	while (1){
		fL *nodeL = (mm+node->left);
		fL *nodeR = (mm+node->right);
		if (node->left != -1 && nodeL->size>=sizeR)
			node = nodeL;
		else if (node->right != -1 && nodeR->size<=sizeR)
			node = nodeR;
		else
			break;
	}
	if (node->size - sizeR < 0 || (node->size == sizeR && node->magic == binaryH)){
		return NULL;
	}
	removeBnode(node);
	fL *newN = addLinked(node, sizeR);
	if (node->size - sizeR != 0) {
		node->size = node->size - sizeR;
		insertBnode(node);
	}
	return newN;
}
void insertBnode (fL *newN){
	if (binaryH == -1){
		binaryH = newN->magic;
		newN->available = 1;
		newN->right = -1;
		newN->left = -1;
		newN->side = -1;
		newN->bprev = -1;
		return;
	}
	if (newN->magic != binaryH) {
		fL *node = (mm+binaryH);
		fL *nodeprev = (mm+binaryH);
		while(1){
			nodeprev = node;    
			if (node->size>newN->size){
				if (node->left == -1)
					break;
				node = (mm+node->left);
			}
			else if (node->size<newN->size){
				if (node->right == -1)
					break;
				node = (mm+node->right);
			}
			else if (node->size==newN->size){
				while (node->side != -1){
					node = (mm+node->side);
				}
				node->side = newN->magic;
				newN->bprev = node->magic;
				newN->right = -1;
				newN->left = -2;
				newN->side = -1;
				return;
			}
		}
		if (newN->magic == 0){
			region = nodeprev->magic;
		}
		if (nodeprev->size>newN->size){
			nodeprev->left=newN->magic;
		}
		else if (nodeprev->size<newN->size){
			nodeprev->right=newN->magic;
		}
		newN->bprev = nodeprev->magic;
		newN->right = -1;
		newN->left = -1;
		newN->side = -1;
	}
}

void removeBnode (fL *node) {
	if (node->magic != binaryH){
		if (node->side != -1 && node->left != -2){
			fL *nodeside = (mm+node->side);
			fL *nodeprv = (mm+node->bprev);
			fL *nodeL = mm+node->left;
			fL *nodeR = mm+node->right;
			if (nodeR->magic == 0 || nodeL->magic == 0){
				region = nodeside->magic;
			}
			nodeside->bprev = node->bprev;
			nodeside->right = node->right;
			nodeside->left = node->left;
			if (nodeprv->left == node->magic)
				nodeprv->left = nodeside->magic;
			else
				nodeprv->right = nodeside->magic;
			nodeL->bprev = nodeside->magic;
			nodeR->bprev = nodeside->magic;
			return;
		}
		
		fL *nodeprv = (mm+node->bprev);
		fL *nodeL = mm+node->left;
		fL *nodeR = mm+node->right;
		if (node->magic == 0){
			nodeprv = (mm+region);
		}
		if (nodeR->magic == 0 || nodeL->magic == 0){
			region = nodeprv->magic;
		}

		if (node->left != -1 && node->right == -1){
			if (node->left == -2){
				fL *nodeside = (mm+node->side);
				nodeside->bprev = nodeprv->magic;
				nodeprv = (mm+node->bprev);
				nodeprv->side=node->side;
				return;
			}
			if (nodeprv->left == node->magic)
				nodeprv->left = node->left;
			else
				nodeprv->right = node->left;
			nodeL->bprev = nodeprv->magic;
			nodeR->bprev = nodeprv->magic; 
		}
		else if (node->left == -1 && node->right != -1){
			if (nodeprv->left == node->magic)
				nodeprv->left = node->right;
			else
				nodeprv->right = node->right;
			nodeL->bprev = nodeprv->magic;
			nodeR->bprev = nodeprv->magic; 
		}
		else if (node->left == -1 && node->right == -1){
			if (nodeprv->left == node->magic)
				nodeprv->left = -1;
			else
				nodeprv->right = -1;
		}
		else {
			fL *pred = node;
			while(pred->left != -1){
				fL *predL = (mm+pred->left);
				pred = predL;
			}
			if (pred->right !=-1){
				fL *predR = (mm+pred->right);
				pred = predR;
			}
			removeBnode(pred);
			pred->bprev = node->bprev;
			if (nodeprv->left == node->magic)
				nodeprv->left = pred->magic;
			else
				nodeprv->right = pred->magic;
			pred->left = node-> left;
			pred->right = node->right;
		} 
	}
	else if (node->magic == binaryH){

		if (node->side!=-1){
			fL *nodeside = (mm+node->side);
			fL *nodeL = mm+node->left;
			fL *nodeR = mm+node->right;
			nodeside->right = node->right;
			nodeside->left = node->left;
			nodeL->bprev = nodeside->magic;
			nodeR->bprev = nodeside->magic;
			binaryH = nodeside->magic;
			return;
		}
		fL *head = mm+binaryH;
		if (head->left == -1 && head->right == -1){
			binaryH = -1;
		}
		else if (head->left == -1 && head->right != -1){
			binaryH = head->right;
		}
		else if (head->left != -1 && head->right == -1){
			binaryH = head->left;
		}
		else {
			fL *pred = head;
			while(pred->left != -1){
				fL *predL = (mm+pred->left);
				pred = predL;
			}
			if (pred->right != -1){
				fL *predR = (mm+pred->right);
				pred = predR;
			}
			binaryH = pred->magic;
			removeBnode(pred);
		}
	}
}

fL *addLinked (fL *node, short size) {
	if (node->size - size == 0){
		node->available = 0;
		return node;
	}
	void *nodevT = (mm+linkedT);
	fL *nodeT = nodevT;
	void *newvN = (mm+node->magic+node->size-size);
	fL *newN = newvN;
	newN->size = size;
	newN->left = -1;
	newN->right = -1;
	newN->side = -1;
	newN->prev = node->magic;
	newN->bprev = -1;
	newN->magic = (node->magic+node->size-size);
	if (node->magic == nodeT->magic){
		linkedT = newN->magic;
	}
	else {
		fL *next = (mm+newN->magic+newN->size);
		next->prev = newN->magic;
	}
	return newN;
}

int Mem_Free(void *ptr) {
	fL *node = (ptr-12);
	if (ptr == NULL || node->size <= 0 || node->available==1)
		return -1;
	fL *nodeprv = (mm+node->prev);
	fL *nodenxt = (mm+node->size+node->magic);
	if (nodenxt->available==1 && nodeprv->available==1) {    
		removeBnode(nodeprv);
		removeBnode(nodenxt);
		fL *nodenxtN = (mm+nodenxt->magic+nodenxt->size);
		nodenxtN->prev = nodeprv->magic;
		nodeprv->size = nodeprv->size+node->size+nodenxt->size;
		insertBnode(nodeprv);
		node->available = 1;
		return 0;
	}
	else if (nodeprv->available==1){
		removeBnode(nodeprv);
		nodenxt->prev = nodeprv->magic;
		nodeprv->size = node->size+nodeprv->size;
		insertBnode(nodeprv);
		node->available = 1;
		return 0;
	}
	else if (nodenxt->available==1){
		removeBnode(nodenxt);
		fL *nodenxtN = (mm+nodenxt->magic+nodenxt->size);
		nodenxtN->prev = node->magic;
		node->size = node->size + nodenxt->size;
		insertBnode(node);
		node->available = 1;
		return 0;
	}
	else {
		insertBnode(node);
		node->available = 1;
		return 0;
	}   
}


void Mem_Dump() {
	int i;
	for (i =0;i <256;i++){
		printf("%d\t%d\t%p\n", *((int *)(mm+i)), i, (mm+i));
	}
}

void initNode(void *node){
	fL *nodeS = node;
	nodeS->size = 0;
	nodeS->magic = 0;
	nodeS->available = 0;
	nodeS->right = -1;
	nodeS->left = -1;
	nodeS->side = -1;
	nodeS->prev = -1;
	nodeS->bprev = -1;
}
