#pragma once

#include <WinSock2.h>

#include <array>

#define SERVER_IP "127.0.0.1"
#define SERVERPORT (9000)

enum eNetworkPacketType : uint32_t
{
	RecvPacket,
	CheckConnect,
	InitGame,
	InputKey,
	NeedNewBlock,
	ResultBlock
};

class FixedPacket
{
public:
	enum
	{
		DATA_SIZE = 100
	};

	FixedPacket(const eNetworkPacketType networkPacketType,
		const uint32_t channelNumber = -1, const uint32_t userNumber = -1)
		: NetworkPacketType(networkPacketType),
		ChannelNumber(channelNumber),
		UserNumber(userNumber)
	{
	}

	eNetworkPacketType NetworkPacketType;
	uint32_t ChannelNumber;
	uint32_t UserNumber;
	std::array<char, DATA_SIZE> Data;
};