#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 VC++ 컴파일 시 경고 방지
#pragma comment(lib, "ws2_32")
#include <winsock2.h>

#include <stdlib.h>
#include <stdio.h>
#include <utility>
#include <thread>
#include <array>
#include <vector>

#include "error.h"
#include "NetworkRecvThread.h"
#include "SocketInfo.h"
#include "ServerLoopThread.h"
#include "FixedPacket.h"
#include "SendGameInint.h"

#define SERVERPORT (9000)
constexpr uint32 RECV_BUFFER_BUFFER_CAPACITY = 10000u;
constexpr uint32 RECV_BUFFER_LENGTH_CAPACITY = 100u;

int main(int argc, char* argv[])
{
	g_RecvBackBuffer.Buffer.reserve(RECV_BUFFER_BUFFER_CAPACITY);
	g_RecvBackBuffer.DataLength.reserve(RECV_BUFFER_LENGTH_CAPACITY);
	g_RecvFrontBuffer.Buffer.reserve(RECV_BUFFER_BUFFER_CAPACITY);
	g_RecvFrontBuffer.DataLength.reserve(RECV_BUFFER_LENGTH_CAPACITY);

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return 1;
	}

	// 입출력 완료 포트 생성
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL)
	{
		return 1;
	}

	// CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si); //(int)si.dwNumberOfProcessors

	

	// 스레드 생성
	for (int i = 0; i < 2; i++)
	{
		HANDLE hThread = CreateThread(NULL, 0, NetworkRecvThread, hcp, 0, NULL);
		if (hThread == NULL)
		{
			return 1;
		}
		else
		{
			CloseHandle(hThread);
		}

		
	}
	
	std::thread serverLoopThread(ServerLoopThread);

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit(TEXT("socket()"));

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	if (bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
	{
		err_quit(TEXT("bind()"));
	}

	// listen()
	if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR)
	{
		err_quit(TEXT("listen()"));
	}

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	DWORD flags;

	uint32 channelCount = 0;
	std::vector<SocketInfo> userRecvSocketInfos;
	std::vector<SocketInfo> userSendSocketInfos;

	while (true)
	{
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display(TEXT("accept()"));
			break;
		}
		printf("[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		// 소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		// 소켓 정보 구조체 할당
		userRecvSocketInfos.emplace_back(client_sock, true);
		userSendSocketInfos.emplace_back(client_sock, false);
		
		Assert(userRecvSocketInfos.size() == userSendSocketInfos.size(), "");
		if (userRecvSocketInfos.size() != Channel::USER_SIZE)
		{
			continue;
		}

		// insert channel in g_ChannelHashMap
		Assert(userRecvSocketInfos.size() == userSendSocketInfos.size(), "");
		Assert((userRecvSocketInfos.size() == Channel::USER_SIZE), "");
		Assert(channelCount != -1, "");
		if(g_ChannelHashMap.find(channelCount) == g_ChannelHashMap.end())
		{
			std::scoped_lock channelHashMapLock(g_ChannelHashMapMutex);

			g_ChannelHashMap[channelCount] = Channel( channelCount, 
				userRecvSocketInfos[0], userSendSocketInfos[0],
				userRecvSocketInfos[1], userSendSocketInfos[1]);
		}
		else
		{
			for (uint32 userNumber = 0; userNumber < Channel::USER_SIZE; ++userNumber)
			{
				closesocket(userRecvSocketInfos[userNumber].mSock);
			}

			userRecvSocketInfos.resize(0);
			userSendSocketInfos.resize(0);

			continue;
		}
		Channel& rChannel = g_ChannelHashMap[channelCount];

		Assert(rChannel.GetChannelNumber() != -1, "");
		std::array<char, sizeof(uint32) * 21> block;
		GetGameInitBlock(block);
		for (uint32 userNumber = 0; userNumber < Channel::USER_SIZE; ++userNumber)
		{
			User& rUser = rChannel.GetUser(userNumber);

			// 데이터 보내기
			SendGameInint(rChannel, rUser, block);

			// 비동기 입출력 시작
			ZeroMemory(&(rUser.GetRecvSocketInfo().mOverlapped), sizeof((rUser.GetRecvSocketInfo().mOverlapped)));
			rUser.GetRecvSocketInfo().mWsabuf.buf = rUser.GetRecvSocketInfo().mBuf;
			rUser.GetRecvSocketInfo().mWsabuf.len = BUFSIZE;
			flags = 0;
			DWORD recvBytes;
			int recvRetVal = WSARecv(rUser.GetRecvSocketInfo().mSock, &rUser.GetRecvSocketInfo().mWsabuf, 1, &recvBytes,
				&flags, &rUser.GetRecvSocketInfo().mOverlapped, NULL);
			if (recvRetVal == SOCKET_ERROR)
			{
				if (WSAGetLastError() != ERROR_IO_PENDING) 
				{
					err_display(TEXT("WSARecv()_main"));
				}
				continue;
			}
		}

		userRecvSocketInfos.resize(0);
		userSendSocketInfos.resize(0);
		++channelCount;
	}

	g_bRunServerLoop = false;
	serverLoopThread.join();

	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}


