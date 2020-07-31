//-----------------------------------------------------------------------------
// Copyright Ed Keenan 2018
// Optimized C++
//----------------------------------------------------------------------------- 

#include "Framework.h"

#include "FreeHdr.h"
#include "UsedHdr.h"

// Add code here

UsedHdr::UsedHdr(const FreeHdr &rFree)
	:
	pUsedNext(0),
	pUsedPrev(0),
	mBlockSize(rFree.mBlockSize),
	mAboveBlockFree(rFree.mAboveBlockFree),
	mBlockType((Type::U8)BlockType::USED),
	pad0(0),
	pad1(0)
{

}

UsedHdr::UsedHdr(const Type::U32 size) {
	this->mBlockSize = size;
}

void UsedHdr::setSize(const Type::U32 size) {
	this->mBlockSize = size;
}

// ---  End of File ---------------
