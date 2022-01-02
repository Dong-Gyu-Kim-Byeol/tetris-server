#include <WinSock2.h>

#include "Assert.h"
#include "Channel.h"
#include "error.h"
#include "ServerLoopThread.h"
#include "FixedPacket.h"




void GetGameInitBlock(std::array<char, sizeof(uint32) * 21>& out_rBlock)
{
	char* pBlockBufPtr = out_rBlock.data();

	// block size
	const uint32 blockSize = 5u;
	*(uint32*)pBlockBufPtr = blockSize;
	pBlockBufPtr += sizeof(uint32);

	// user 0 block
	for (uint32_t i = 0; i < blockSize; ++i)
	{
		*(uint32*)pBlockBufPtr = (uint32)rand() % 7;
		pBlockBufPtr += sizeof(uint32);
		*(uint32*)pBlockBufPtr = (uint32)((rand() % 6) + 101);
		pBlockBufPtr += sizeof(uint32);
	}

	// user 1 block
	for (uint32_t i = 0; i < blockSize; ++i)
	{
		*(uint32*)pBlockBufPtr = (uint32)rand() % 7;
		pBlockBufPtr += sizeof(uint32);
		*(uint32*)pBlockBufPtr = (uint32)((rand() % 6) + 101);
		pBlockBufPtr += sizeof(uint32);
	}
}

void SendGameInint(Channel& rChannel, User& out_rUser, const std::array<char, sizeof(uint32) * 21>& rBlock)
{
	FixedPacket sendBuf(eNetworkPacketType::InitGame,
		rChannel.GetChannelNumber(), out_rUser.GetUserNumber());
	char* pSenfBufPtr = sendBuf.Data.data();
	
	// other USER_SIZE
	*(uint32*)pSenfBufPtr = (uint32)Channel::USER_SIZE - 1;
	pSenfBufPtr += sizeof(uint32);
	uint32* pOtherUserNumber = (uint32*)pSenfBufPtr;
	
	// other user num
	for (uint32 i = 0; i < Channel::USER_SIZE; ++i)
	{
		if (out_rUser.GetUserNumber() == rChannel.GetUser(i).GetUserNumber())
		{
			continue;
		}

		*(uint32*)pSenfBufPtr = (uint32)(rChannel.GetUser(i).GetUserNumber());
		pSenfBufPtr += sizeof(uint32);
	}
	
	memcpy(pSenfBufPtr, rBlock.data(), rBlock.size());
	pSenfBufPtr += rBlock.size();
	
	size_t size = ((size_t)(pSenfBufPtr)-(size_t)(sendBuf.Data.data()));
	Assert(size <= sendBuf.Data.size(), "");

	WSABUF wsabuf;
	wsabuf.buf = (char*)&sendBuf;
	wsabuf.len = sizeof(sendBuf);
	Send(out_rUser.GetSendSocketInfo().mSock, wsabuf, out_rUser.GetSendSocketInfo().mOverlapped);
}
