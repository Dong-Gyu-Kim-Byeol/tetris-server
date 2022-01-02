#pragma once

#include <vector>

#include "Typedef.h"

class NetworkBuffer final
{
public:
	std::vector<uint32> DataLength;
	std::vector<char> Buffer;

	void Reset()
	{
		DataLength.resize(0u);
		Buffer.resize(0u);
	}

	void Swap(NetworkBuffer& out_rNetworkBuffer)
	{
		DataLength.swap(out_rNetworkBuffer.DataLength);
		Buffer.swap(out_rNetworkBuffer.Buffer);
	}
};