#pragma once
#include <tchar.h>

// 소켓 함수 오류 출력 후 종료
void err_quit(const TCHAR* pMsg);

// 소켓 함수 오류 출력
void err_display(const TCHAR* pMsg);