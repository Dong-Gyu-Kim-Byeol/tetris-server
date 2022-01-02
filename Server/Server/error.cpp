#include "error.h"

#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <tchar.h>

void err_quit(const TCHAR* pMsg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    //MessageBox(NULL, (LPCTSTR)lpMsgBuf, pMsg, MB_ICONERROR);
    _tprintf(TEXT("[%s] %s"), pMsg, lpMsgBuf);
    LocalFree(lpMsgBuf);
    exit(1);
}

// 소켓 함수 오류 출력
void err_display(const TCHAR* pMsg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    //MessageBox(NULL, (LPCTSTR)lpMsgBuf, pMsg, MB_ICONERROR);
    _tprintf(TEXT("[%s] %s"), pMsg, lpMsgBuf);

    LocalFree(lpMsgBuf);
}