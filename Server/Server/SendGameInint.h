#pragma once

#include "Channel.h"

void Send(SOCKET sock, WSABUF& rWsabuf);

void SendGameInint(Channel& rChannel, User& out_rUser, const std::array<char, sizeof(uint32) * 21>& rBlock);

void GetGameInitBlock(std::array<char, sizeof(uint32) * 21>& out_rBlock);