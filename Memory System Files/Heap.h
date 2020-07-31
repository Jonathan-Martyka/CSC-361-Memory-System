//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2018
// Optimized C++
//----------------------------------------------------------------------------- 

#ifndef HEAPHDR_H
#define HEAPHDR_H

#include "Types.h"

#include "UsedHdr.h"
#include "FreeHdr.h"

class Heap
{
public:
	struct Stats
	{
		Type::UInt peakNumUsed;       // number of peak used allocations
		Type::UInt peakUsedMemory;    // peak size of used memory

		Type::UInt currNumUsedBlocks; // number of current used allocations
		Type::UInt currUsedMem;       // current size of the total used memory

		Type::UInt currNumFreeBlocks; // number of current free blocks
		Type::UInt currFreeMem;       // current size of the total free memory

		Type::UInt sizeHeap;          // size of Heap total space, including header

		void *heapTopAddr;            // start address available heap
		void *heapBottomAddr;         // bottom of address of heap
	};

public:
	// Make sure that the Heap is 16 byte aligned.

	// allocation links
	UsedHdr		*pUsedHead;
	FreeHdr		*pFreeHead;

	// Next fit allocation strategy
	FreeHdr		*pNextFit;	

	// Using int for speed
	Type::U32	mInitialize;

	// stats
	Stats		mStats;

	// Padding to guarantee 16 byte alignment
	Type::U32	pad1;
	Type::U32	pad2;
	Type::U32	pad3;

	// specialize constructor
	Heap(void * ptr);
};

#endif 

// ---  End of File ---------------
