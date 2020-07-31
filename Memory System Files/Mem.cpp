//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2018
// Optimized C++
//----------------------------------------------------------------------------- 

#include <malloc.h>
#include <new>

#include "Framework.h"

#include "Mem.h"
#include "Heap.h"
#include "BlockType.h"

#define STUB_PLEASE_REPLACE(x) (x)

#define HEAP_ALIGNMENT			16
#define HEAP_ALIGNMENT_MASK		(HEAP_ALIGNMENT-1)

#define ALLOCATION_ALIGNMENT		16
#define ALLOCATION_ALIGNMENT_MASK	(ALLOCATION_ALIGNMENT-1)

#define UNUSED_VAR(v)  ((void *)v)

#ifdef _DEBUG
#define HEAP_HEADER_GUARDS  16
#define HEAP_SET_GUARDS  	Type::U32 *pE = (Type::U32 *)((Type::U32)pRawMem + HEAP_SIZE); \
								*pE++ = 0xEEEEEEEE;*pE++ = 0xEEEEEEEE;*pE++ = 0xEEEEEEEE;*pE++ = 0xEEEEEEEE;
#define HEAP_TEST_GUARDS	Type::U32 *pE = (Type::U32 *)((Type::U32)pRawMem + HEAP_SIZE); \
								assert(*pE++ == 0xEEEEEEEE);assert(*pE++ == 0xEEEEEEEE); \
								assert(*pE++ == 0xEEEEEEEE);assert(*pE++ == 0xEEEEEEEE);  
#else
#define HEAP_HEADER_GUARDS  0
#define HEAP_SET_GUARDS  	
#define HEAP_TEST_GUARDS			 
#endif


// To help with coalescing... not required
struct SecretPtr
{
	FreeHdr *free;
};


Mem::~Mem()
{
	HEAP_TEST_GUARDS
		_aligned_free(this->pRawMem);
}


Heap *Mem::GetHeap()
{
	return this->pHeap;
}

Mem::Mem()
{
	// now initialize it.
	this->pHeap = 0;
	this->pRawMem = 0;

	// Do a land grab --- get the space for the whole heap
	// Since OS have different alignments... I forced it to 16 byte aligned
	pRawMem = _aligned_malloc(HEAP_SIZE + HEAP_HEADER_GUARDS, HEAP_ALIGNMENT);
	HEAP_SET_GUARDS

		// verify alloc worked
		assert(pRawMem != 0);

	// Guarantee alignemnt
	assert(((Type::U32)pRawMem & HEAP_ALIGNMENT_MASK) == 0x0);

	// instantiate the heap header on the raw memory
	Heap *p = new(pRawMem) Heap(pRawMem);

	// update it
	this->pHeap = p;
}


void Mem::Initialize()
{
	// Add magic here

	void *pAddr = this->pHeap + 1;

	FreeHdr *pFree = new(pAddr) FreeHdr(pAddr, pHeap->mStats.heapBottomAddr);

	this->pHeap->pFreeHead = pFree;
	this->pHeap->pNextFit = pFree;

	this->pHeap->mStats.currFreeMem = pFree->mBlockSize;
	this->pHeap->mStats.currNumFreeBlocks = 1;

}


void *Mem::Malloc(const Type::U32 size)
{
	FreeHdr *pFree = this->pHeap->pNextFit;
	FreeHdr *pStart = pFree;

	if (this->pHeap->pNextFit == 0) return 0;

	if (pFree->mBlockSize < size) {

		do {
			if (pFree->mBlockSize >= size) {
				break;
			}

			if (pFree->pFreeNext == 0) pFree = pHeap->pFreeHead;
			else pFree = pFree->pFreeNext;
		} while (pFree != pStart);
		if (pFree == pStart) return 0;
	}


	//Splitting Free into new Free below if space allows

	const Type::U32 tmpSize = pFree->mBlockSize;

	if (size + sizeof(FreeHdr) + sizeof(SecretPtr) < tmpSize) {
		//Case where there is spare room and spare Free is made
		Type::U8 *pEndUsed = ((Type::U8 *)((UsedHdr *)pFree + 1)) + size;
		Type::U32 spareSize = tmpSize;
		FreeHdr *newFree = new(pEndUsed) FreeHdr((Type::U8 *)(pFree + 1) + spareSize);
		SecretPtr *newSecret = new((Type::U8 *)(newFree + 1) + newFree->mBlockSize - sizeof(SecretPtr)) SecretPtr();
		newSecret->free = newFree;

		if (this->pHeap->pFreeHead == 0) {
			this->pHeap->pFreeHead = newFree;
			this->pHeap->pNextFit = this->pHeap->pFreeHead;
		}

		if (pFree == this->pHeap->pFreeHead) {
			this->pHeap->pFreeHead = newFree;
		}

		newFree->pFreeNext = pFree->pFreeNext;
		newFree->pFreePrev = pFree->pFreePrev;
		if (pFree->pFreeNext != 0) pFree->pFreeNext->pFreePrev = newFree;
		if (pFree->pFreePrev != 0) pFree->pFreePrev->pFreeNext = newFree;

		this->pHeap->pNextFit = newFree;

		this->pHeap->mStats.currNumFreeBlocks++;
		this->pHeap->mStats.currFreeMem -= sizeof(FreeHdr);
	}
	else {
		if (pFree == this->pHeap->pFreeHead) {
			this->pHeap->pFreeHead = pFree->pFreeNext;
		}

		if (pFree->pFreeNext != 0) pFree->pFreeNext->pFreePrev = pFree->pFreePrev;
		if (pFree->pFreePrev != 0) pFree->pFreePrev->pFreeNext = pFree->pFreeNext;

		this->pHeap->pNextFit = pFree->pFreeNext;
	}

	//Actual creation of new UsedHdr
	UsedHdr *newUsed = new(pFree) UsedHdr(*pFree);
	newUsed->setSize(size);



	this->pHeap->mStats.currNumUsedBlocks++;
	if (this->pHeap->mStats.currNumUsedBlocks > this->pHeap->mStats.peakNumUsed)
		this->pHeap->mStats.peakNumUsed = this->pHeap->mStats.currNumUsedBlocks;

	this->pHeap->mStats.currUsedMem += newUsed->mBlockSize;
	if (this->pHeap->mStats.currUsedMem > this->pHeap->mStats.peakUsedMemory)
		this->pHeap->mStats.peakUsedMemory = this->pHeap->mStats.currUsedMem;

	this->pHeap->mStats.currNumFreeBlocks--;
	this->pHeap->mStats.currFreeMem -= newUsed->mBlockSize;


	if (this->pHeap->pUsedHead == 0) {
		this->pHeap->pUsedHead = newUsed;
	}
	else {
		newUsed->pUsedNext = this->pHeap->pUsedHead;
		newUsed->pUsedNext->pUsedPrev = newUsed;
		this->pHeap->pUsedHead = newUsed;
	}

	return newUsed + 1;

}


void Mem::Free(void * const data)
{
	UsedHdr *pUsed = (UsedHdr *)data;
	pUsed--;

	if (pUsed == 0) return;

	const Type::U32 memFreed = pUsed->mBlockSize;

	if (this->pHeap->pUsedHead == pUsed) {
		this->pHeap->pUsedHead = pUsed->pUsedNext;
	}

	if (pUsed->pUsedNext != 0) pUsed->pUsedNext->pUsedPrev = pUsed->pUsedPrev;
	if (pUsed->pUsedPrev != 0) pUsed->pUsedPrev->pUsedNext = pUsed->pUsedNext;

	FreeHdr *newFree = new(pUsed) FreeHdr(*pUsed);
	bool comBelow = false;

	if (this->pHeap->pFreeHead == 0) {
		this->pHeap->pFreeHead = newFree;
		this->pHeap->pNextFit = this->pHeap->pFreeHead;
	}
	else {
		
		
		
		if ((UsedHdr *)((Type::U8 *)(newFree + 1) + newFree->mBlockSize) < this->pHeap->mStats.heapBottomAddr && ((UsedHdr *)((Type::U8 *)(newFree + 1) + newFree->mBlockSize))->mBlockType == (Type::U8)BlockType::FREE) {
			
			FreeHdr * pFollow = (FreeHdr*)((Type::U8 *)(newFree + 1) + newFree->mBlockSize);

			if (this->pHeap->pFreeHead == pFollow) this->pHeap->pFreeHead = newFree;
			if (this->pHeap->pNextFit == pFollow) this->pHeap->pNextFit = newFree;

			if (pFollow->pFreeNext != 0) pFollow->pFreeNext->pFreePrev = newFree;
			if (pFollow->pFreePrev != 0) pFollow->pFreePrev->pFreeNext = newFree;
			newFree->pFreePrev = pFollow->pFreePrev;
			newFree->pFreeNext = pFollow->pFreeNext;

			newFree->mBlockSize += pFollow->mBlockSize + sizeof(FreeHdr);
			this->pHeap->mStats.currNumFreeBlocks--;
			this->pHeap->mStats.currFreeMem += sizeof(FreeHdr);

			comBelow = true;

		}

		if (newFree->mAboveBlockFree != 0){
			

			FreeHdr * pPrev = (((SecretPtr *)newFree) - 1)->free;

			pPrev->mBlockSize += newFree->mBlockSize + sizeof(FreeHdr);
			//Cases only occurs if combining from both sides at once
			if (pPrev->pFreeNext == newFree) pPrev->pFreeNext = newFree->pFreeNext;
			if (this->pHeap->pNextFit == newFree) this->pHeap->pNextFit = pPrev;

			newFree = pPrev;

			this->pHeap->mStats.currNumFreeBlocks--;
			this->pHeap->mStats.currFreeMem += sizeof(FreeHdr);
		}
		else if (newFree < this->pHeap->pFreeHead) {
			newFree->pFreeNext = this->pHeap->pFreeHead;
			this->pHeap->pFreeHead->pFreePrev = newFree;
			this->pHeap->pFreeHead = newFree;
		}
		else if(newFree != this->pHeap->pFreeHead && !comBelow) {

			FreeHdr* checkFree = this->pHeap->pFreeHead;

			while (checkFree->pFreeNext != 0 && checkFree->pFreeNext < newFree) {
				checkFree = checkFree->pFreeNext;
			}

			if (checkFree->pFreeNext != 0) checkFree->pFreeNext->pFreePrev = newFree;
			newFree->pFreeNext = checkFree->pFreeNext;
			newFree->pFreePrev = checkFree;
			checkFree->pFreeNext = newFree;

		}



	}

	if ((UsedHdr *)((Type::U8 *)(newFree + 1) + newFree->mBlockSize) < this->pHeap->mStats.heapBottomAddr)
		((UsedHdr *)((Type::U8 *)(newFree + 1) + newFree->mBlockSize))->mAboveBlockFree = true;

	SecretPtr *newSecret = new((Type::U8 *)(newFree + 1) + newFree->mBlockSize - sizeof(SecretPtr)) SecretPtr();
	newSecret->free = newFree;

	this->pHeap->mStats.currNumUsedBlocks--;
	this->pHeap->mStats.currUsedMem -= memFreed;

	this->pHeap->mStats.currNumFreeBlocks++;
	this->pHeap->mStats.currFreeMem += memFreed;

	/*
	if (newFree->pFreeNext != 0 && (Type::U8 *)newFree->pFreeNext == (Type::U8 *)(newFree + 1) + newFree->mBlockSize) {
	if (newFree->pFreeNext->pFreeNext != 0) newFree->pFreeNext->pFreeNext->pFreePrev = newFree;
	newFree->mBlockSize += newFree->pFreeNext->mBlockSize + sizeof(FreeHdr);
	newFree->pFreeNext = newFree->pFreeNext->pFreeNext;
	}
	*/

}


void Mem::Dump()
{

	fprintf(FileIO::GetHandle(), "\n------- DUMP -------------\n\n");

	fprintf(FileIO::GetHandle(), "heapStart: 0x%p     \n", this->pHeap);
	fprintf(FileIO::GetHandle(), "  heapEnd: 0x%p   \n\n", this->pHeap->mStats.heapBottomAddr);
	fprintf(FileIO::GetHandle(), "pUsedHead: 0x%p     \n", this->pHeap->pUsedHead);
	fprintf(FileIO::GetHandle(), "pFreeHead: 0x%p     \n", this->pHeap->pFreeHead);
	fprintf(FileIO::GetHandle(), " pNextFit: 0x%p   \n\n", this->pHeap->pNextFit);

	fprintf(FileIO::GetHandle(), "Heap Hdr   s: %p  e: %p                            size: 0x%x \n", (void *)((Type::U32)this->pHeap->mStats.heapTopAddr - sizeof(Heap)), this->pHeap->mStats.heapTopAddr, sizeof(Heap));

	Type::U32 p = (Type::U32)pHeap->mStats.heapTopAddr;

	char *type;
	char *typeHdr;

	while (p < (Type::U32)pHeap->mStats.heapBottomAddr)
	{
		UsedHdr *used = (UsedHdr *)p;
		if (used->mBlockType == (Type::U8)BlockType::USED)
		{
			typeHdr = "USED HDR ";
			type = "USED     ";
		}
		else
		{
			typeHdr = "FREE HDR ";
			type = "FREE     ";
		}

		Type::U32 hdrStart = (Type::U32)used;
		Type::U32 hdrEnd = (Type::U32)used + sizeof(UsedHdr);
		fprintf(FileIO::GetHandle(), "%s  s: %p  e: %p  p: %p  n: %p  size: 0x%x    AF: %d \n", typeHdr, (void *)hdrStart, (void *)hdrEnd, used->pUsedPrev, used->pUsedNext, sizeof(UsedHdr), used->mAboveBlockFree);
		Type::U32 blkStart = hdrEnd;
		Type::U32 blkEnd = blkStart + used->mBlockSize;
		fprintf(FileIO::GetHandle(), "%s  s: %p  e: %p                            size: 0x%x \n", type, (void *)blkStart, (void *)blkEnd, used->mBlockSize);

		p = blkEnd;

	}
}

// ---  End of File ---------------
