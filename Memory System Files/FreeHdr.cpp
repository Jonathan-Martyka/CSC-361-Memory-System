//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2018
// Optimized C++
//----------------------------------------------------------------------------- 

#include "Framework.h"

#include "UsedHdr.h"
#include "FreeHdr.h"
#include "BlockType.h"

// add code here

//For initial free on whole heap
FreeHdr::FreeHdr(void * Top, void * Bottom)
	:pFreeNext(0),
	pFreePrev(0),
	mBlockType((Type::U8)BlockType::FREE),
	mAboveBlockFree(false),
	pad1(0),
	pad2(0)
{

	//Set block size
	Type::U32 TotalSize = ((Type::U32)Bottom - (Type::U32)Top);
	this->mBlockSize = TotalSize - sizeof(FreeHdr);
}


FreeHdr::FreeHdr(const void * const pBottom)
	:pFreeNext(0),
	pFreePrev(0),
	mBlockType((Type::U8)BlockType::FREE),
	mAboveBlockFree(false),
	pad1(0),
	pad2(0)
{
	//Set block size
	Type::U32 TotalSize = ((Type::U32)pBottom - (Type::U32)this);
	this->mBlockSize = TotalSize - sizeof(FreeHdr);
}

//For converting UsedHdr to FreeHdr
FreeHdr::FreeHdr(const UsedHdr & rUsed)
	:pFreeNext(0),
	pFreePrev(0),
	mBlockSize(rUsed.mBlockSize),
	mAboveBlockFree(rUsed.mAboveBlockFree),
	mBlockType((Type::U8)BlockType::FREE),
	pad1(0),
	pad2(0)
{

}

// ---  End of File ---------------
