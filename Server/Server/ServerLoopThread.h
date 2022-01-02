#pragma once
#include <mutex>
#include <unordered_map>
#include <WinSock2.h>

class Channel;
class FixedPacket;
#include "NetworkBuffer.h"

extern std::mutex g_RecvBufferMutex;
extern NetworkBuffer g_RecvBackBuffer;
extern NetworkBuffer g_RecvFrontBuffer;

extern std::mutex g_ChannelHashMapMutex;
extern std::unordered_map<uint32, Channel> g_ChannelHashMap;
extern std::mutex g_DeleteChannelNumberMutex;
extern std::vector<uint32> g_DeleteChannelNumber;

extern bool g_bRunServerLoop;

void ServerLoopThread();

void SendChannelBroadcast(Channel& out_rChannel, const FixedPacket& rPacket);
void ProcessPacket(const FixedPacket& rPacket, Channel& out_rChannel);
void ProcessRecvFrontBuffer();
void DeleteChannel();
void Send(SOCKET sock, WSABUF& rWsabuf, OVERLAPPED& rOverlapped);