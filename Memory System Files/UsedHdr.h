//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2018
// Optimized C++
//----------------------------------------------------------------------------- 

#ifndef USEDHDR_H
#define USEDHDR_H

#include "Types.h"
#include "BlockType.h"

class FreeHdr;

class UsedHdr
{
public:

	UsedHdr * pUsedNext;       // next used block
	UsedHdr		*pUsedPrev;		  // prev used block
	Type::U32   mBlockSize;		  // size of block
	Type::Bool	mAboveBlockFree;  // AboveBlock flag
								  //    if(AboveBlock is type free) -> true 
								  //    if(AboveBlock is type used) -> false
	Type::U8	mBlockType;       // block type 
	Type::U8	pad0;             // future use
	Type::U8	pad1;			  // future use

	explicit UsedHdr(const FreeHdr &rFree);
	explicit UsedHdr(const Type::U32 size);

	void setSize(const Type::U32 size);
};

#endif 

// ---  End of File ---------------
