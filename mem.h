#ifndef __MEM_h__
#define __MEM_h__

int Mem_Init(int sizeOfRegion);
void *Mem_Alloc(int size);
int Mem_Free(void *ptr);
void Mem_Dump();


#endif // __MEM_h__
