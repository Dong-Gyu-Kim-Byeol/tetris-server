#pragma once
#include <WinSock2.h>
// 작업자 스레드 함수
DWORD WINAPI NetworkRecvThread(LPVOID arg);