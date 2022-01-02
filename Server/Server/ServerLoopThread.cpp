#include "ServerLoopThread.h"

#include <WinSock2.h>

#include "Typedef.h"
#include "error.h"
#include "Assert.h"
#include "Channel.h"
#include "FixedPacket.h"

std::mutex g_RecvBufferMutex;
NetworkBuffer g_RecvBackBuffer;
NetworkBuffer g_RecvFrontBuffer;

std::mutex g_ChannelHashMapMutex;
std::unordered_map<uint32, Channel> g_ChannelHashMap;
std::mutex g_DeleteChannelNumberMutex;
std::vector<uint32> g_DeleteChannelNumber;

bool g_bRunServerLoop = true;

void Send(SOCKET sock, WSABUF& rWsabuf, OVERLAPPED& rOverlapped)
{
	ZeroMemory(&rOverlapped, sizeof(rOverlapped));

	DWORD sendBytes;
	int sendRetVal = WSASend(sock, &rWsabuf, 1,
		&sendBytes, 0, &rOverlapped, NULL);
	if (sendRetVal == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			err_display(TEXT("WSASend()_main"));
		}
	}
}

void SendChannelBroadcast(Channel& out_rChannel, const FixedPacket& rPacket)
{
	// send
	for (uint32 userNumber = 0; userNumber < Channel::USER_SIZE; ++userNumber)
	{
		User& rUser = out_rChannel.GetUser(userNumber);

		ZeroMemory(&(rUser.GetSendSocketInfo().mOverlapped), sizeof(rUser.GetSendSocketInfo().mOverlapped));
		rUser.GetSendSocketInfo().mWsabuf.buf = (char*)&rPacket;
		rUser.GetSendSocketInfo().mWsabuf.len = sizeof(rPacket);

		int retval = WSASend(rUser.GetSendSocketInfo().mSock, &rUser.GetSendSocketInfo().mWsabuf, 1,
			nullptr, 0, &(rUser.GetSendSocketInfo().mOverlapped), NULL);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				err_display(TEXT("WSASend()_SendChannelBroadcast"));
			}
		}

		//
		std::cout << rUser.GetSendSocketInfo().mSock << " / " << rUser.GetChannelNumber() << std::endl;
		//
	}
}

void ProcessPacket(const FixedPacket& rPacket, Channel& out_rChannel)
{
	switch (rPacket.NetworkPacketType)
	{

	case eNetworkPacketType::RecvPacket:
	{
		Assert(false, "");
	}
	break;
	case eNetworkPacketType::InitGame:
	{
		Assert(false, "");
	}
	break;
	case eNetworkPacketType::InputKey:
	{
		std::cout << "InputKey" << std::endl;
		SendChannelBroadcast(out_rChannel, rPacket);
	}
	break;
	case eNetworkPacketType::NeedNewBlock:
	{
		std::cout << "NeedNewBlock" << std::endl;
		FixedPacket sendPacket(eNetworkPacketType::ResultBlock,
			rPacket.ChannelNumber, rPacket.UserNumber);
		char* pSenfBufPtr = sendPacket.Data.data();

		*(uint32*)pSenfBufPtr = (uint32)rand() % 7;
		pSenfBufPtr += sizeof(uint32);
		*(uint32*)pSenfBufPtr = (uint32)((rand() % 6) + 101);
		pSenfBufPtr += sizeof(uint32);

		SendChannelBroadcast(out_rChannel, sendPacket);
	}
	break;
	case eNetworkPacketType::ResultBlock:
	{
		Assert(false, "");
	}
	break;
	default:
		Assert(false, "");
		break;
	}
}

void ProcessRecvFrontBuffer()
{
	// 데이터 보내기
	const uint32 packetCount = static_cast<uint32>(g_RecvFrontBuffer.DataLength.size());
	uint32 dataLength = 0;
	for (uint32 packetNumber = 0; packetNumber < packetCount; ++packetNumber)
	{
		const FixedPacket& rPacket = *(FixedPacket*)(g_RecvFrontBuffer.Buffer.data() + dataLength);
		dataLength += g_RecvFrontBuffer.DataLength[packetNumber];

		Channel* pChannel;
		{
			std::scoped_lock channelHashMapLock(g_ChannelHashMapMutex);

			if (g_ChannelHashMap.find(rPacket.ChannelNumber) == g_ChannelHashMap.end())
			{
				packetNumber = packetCount;
				continue;
			}
			pChannel = &(g_ChannelHashMap[rPacket.ChannelNumber]);
		}
		Channel rChannel = *pChannel;

		ProcessPacket(rPacket, rChannel);
	}
}

void DeleteChannel()
{
	{
		std::scoped_lock g_DeleteChannelLock(g_ChannelHashMapMutex, g_DeleteChannelNumberMutex);

		for (uint32 deleteChannelNumber : g_DeleteChannelNumber)
		{
			Channel& rDeleteChannel = g_ChannelHashMap[deleteChannelNumber];
			rDeleteChannel.Delete();

			g_ChannelHashMap.erase(deleteChannelNumber);
		}

		g_DeleteChannelNumber.resize(0);
	}
}

void ServerLoopThread()
{
	static_assert(sizeof(char) == sizeof(uint8));

	auto start = std::chrono::high_resolution_clock::now();
	constexpr double fpsTime = 1.0 / 60.0;

	while (true)
	{
		if (g_bRunServerLoop == false)
		{
			return;
		}

		if (static_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start).count() < fpsTime)
		{
			continue;
		}

		if (g_ChannelHashMap.size() == 0)
		{
			continue;
		}

		DeleteChannel();

		ProcessRecvFrontBuffer();

		// NetworkBuffer Swap
		{
			std::scoped_lock recvBufferLock(g_RecvBufferMutex);

			g_RecvFrontBuffer.Reset();
			g_RecvFrontBuffer.Swap(g_RecvBackBuffer);
		}
	}
}