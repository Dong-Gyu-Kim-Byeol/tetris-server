#pragma once
#include <winsock2.h>

#include "Assert.h"

#define BUFSIZE    (256)

// ���� ���� ������ ���� ����ü
class SocketInfo
{
public:
    SocketInfo(SOCKET sock, bool bRecvInfo) :
        mSock(sock),
        mbRecvInfo(bRecvInfo),
        mRecvBytes(0),
        mOverlapped{ 0, }
    {
        Assert((void*)this == (void*)(&mOverlapped), "");
    }

    SocketInfo() :
        mSock(-1),
        mbRecvInfo(false),
        mRecvBytes(0),
        mOverlapped{ 0, }
    {
        Assert((void*)this == (void*)(&mOverlapped), "");
    }

public:
    OVERLAPPED mOverlapped;
    WSABUF mWsabuf;
    SOCKET mSock;
    int mRecvBytes;
    bool mbRecvInfo;
    char mBuf[BUFSIZE];
};