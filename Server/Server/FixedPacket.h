#pragma once

#include <array>

#include "Typedef.h"

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

	bool CheckPacketType()
	{
		switch (NetworkPacketType)
		{
		case eNetworkPacketType::InputKey:
			return true;
			break;
		case eNetworkPacketType::NeedNewBlock:
			return true;
			break;
		default:
			return false;
		}
	}

	eNetworkPacketType NetworkPacketType;
	uint32_t ChannelNumber;
	uint32_t UserNumber;
	std::array<char, DATA_SIZE> Data;
};