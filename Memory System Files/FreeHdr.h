//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2018
// Optimized C++
//----------------------------------------------------------------------------- 

#ifndef FREEHDR_H
#define FREEHDR_H

#include "Types.h"
#include "BlockType.h"

class UsedHdr;

class FreeHdr
{
public:

	FreeHdr(void * Top, void * Bottom);

	FreeHdr     *pFreeNext;       // next free block
	FreeHdr     *pFreePrev;       // prev free block
	Type::U32   mBlockSize;       // size of block
	Type::Bool	mAboveBlockFree;  // AboveBlock flag
								  //    if(AboveBlock is type free) -> true 
								  //    if(AboveBlock is type used) -> false
	Type::U8	mBlockType;       // block type 
	Type::U8	pad1;             // future use
	Type::U8	pad2;             // future use


	explicit FreeHdr(const void * const pBottom);
	explicit FreeHdr(const UsedHdr & rUsed);
};

#endif 

// ---  End of File ---------------
