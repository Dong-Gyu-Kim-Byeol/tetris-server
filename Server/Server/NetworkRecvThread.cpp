#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#include "NetworkRecvThread.h"

#include <stdio.h>

#include "SOCKETINFO.h"
#include "error.h"
#include "Assert.h"
#include "Channel.h"
#include "ServerLoopThread.h"
#include "FixedPacket.h"



// �۾��� ������ �Լ�
DWORD WINAPI NetworkRecvThread(LPVOID arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;

	while (1)
	{
		// �񵿱� ����� �Ϸ� ��ٸ���
		DWORD cbTransferred;
		SOCKET client_sock;
		OVERLAPPED* pOverlapped;
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred,
			(PULONG_PTR)&client_sock, &pOverlapped, INFINITE);
		Assert(pOverlapped != nullptr, "");

		// Ŭ���̾�Ʈ ���� ���
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

		SocketInfo& rSocketInfo = *(SocketInfo*)pOverlapped;
		User* pUser = User::GetUserPointer(rSocketInfo);

		// �񵿱� ����� ��� Ȯ��
		if (retval == 0 || cbTransferred == 0)
		{
			if (retval == 0)
			{
				DWORD temp1, temp2;
				WSAGetOverlappedResult(client_sock, pOverlapped,
					&temp1, FALSE, &temp2);
				err_display(TEXT("WSAGetOverlappedResult()"));
			}

			printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
				inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

			if (g_ChannelHashMap.size() == 0)
			{
				continue;
			}

			// Delete Channel
			{
				std::scoped_lock g_DeleteChannelNumberLock(g_DeleteChannelNumberMutex);

				g_DeleteChannelNumber.push_back(pUser->GetChannelNumber());
			}

			continue;
		}

		if (rSocketInfo.mbRecvInfo == false)
		{
			continue;
		}
		SocketInfo& rRecvSocketInfo = rSocketInfo;

		//
		std::cout << client_sock << " R  " << pUser->GetChannelNumber() << std::endl;
		//

		Assert(cbTransferred <= sizeof(FixedPacket), "packet size is large.");
		rRecvSocketInfo.mRecvBytes += cbTransferred;

		// ���� �������� ũ�� �̴�
		if (rRecvSocketInfo.mRecvBytes < sizeof(FixedPacket))
		{
			// ���� ������ ���
			//rSocketInfo.buf[rSocketInfo.recvbytes] = '\0';
			//printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
			//	ntohs(clientaddr.sin_port), &(rSocketInfo.buf[rSocketInfo.recvbytes - cbTransferred]));

			ZeroMemory(&rRecvSocketInfo.mOverlapped, sizeof(rRecvSocketInfo.mOverlapped));
			rRecvSocketInfo.mWsabuf.buf = rRecvSocketInfo.mBuf + rRecvSocketInfo.mRecvBytes;
			rRecvSocketInfo.mWsabuf.len = sizeof(FixedPacket) - rRecvSocketInfo.mRecvBytes;

			DWORD flags = 0;
			retval = WSARecv(rRecvSocketInfo.mSock, &rRecvSocketInfo.mWsabuf, 1,
				nullptr, &flags, &rRecvSocketInfo.mOverlapped, NULL);
			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					err_display(TEXT("WSARecv()_1"));
				}
			}
			continue;
		}

		if (rRecvSocketInfo.mRecvBytes != sizeof(FixedPacket))
		{
			continue;
		}

		FixedPacket* pPacket = (FixedPacket*)rRecvSocketInfo.mBuf;
		if (pPacket->CheckPacketType() == true)
		{
			// RecvBuffer
			{
				std::scoped_lock recvBufferLock(g_RecvBufferMutex);

				g_RecvBackBuffer.DataLength.push_back(rRecvSocketInfo.mRecvBytes);

				if (g_RecvBackBuffer.Buffer.capacity() - g_RecvBackBuffer.Buffer.size() < sizeof(FixedPacket))
				{
					g_RecvBackBuffer.Buffer.reserve(g_RecvBackBuffer.Buffer.capacity() * 2);
				}
				size_t bufLen = g_RecvBackBuffer.Buffer.size();
				g_RecvBackBuffer.Buffer.resize(bufLen + sizeof(FixedPacket));
				memcpy(g_RecvBackBuffer.Buffer.data() + bufLen, rRecvSocketInfo.mBuf, sizeof(FixedPacket));
			}
		}
		rRecvSocketInfo.mRecvBytes = 0;

		// ������ �ޱ�
		ZeroMemory(&rRecvSocketInfo.mOverlapped, sizeof(rRecvSocketInfo.mOverlapped));
		rRecvSocketInfo.mWsabuf.buf = rRecvSocketInfo.mBuf;
		rRecvSocketInfo.mWsabuf.len = sizeof(FixedPacket);

		DWORD flags = 0;
		retval = WSARecv(rRecvSocketInfo.mSock, &rRecvSocketInfo.mWsabuf, 1,
			nullptr, &flags, &rRecvSocketInfo.mOverlapped, NULL);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				err_display(TEXT("WSARecv()_2"));
			}
			continue;
		}
	}


	return 0;
}