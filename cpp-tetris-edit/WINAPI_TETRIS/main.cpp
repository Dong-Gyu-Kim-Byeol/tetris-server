#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "Msimg32.lib")

#include <array>
#include <chrono>
#include <queue>
#include <cassert>

#include "헤더.h"
#include "GameScene.h"
#include "resource.h"


//2조 추가
#include "network.h"
#include "error.h"



//
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
const char* lpszClass = "TETRIS 2018";



GameScene g_GameScene;
// 2조 추가
OtherGameScene g_OtherGameScene;
HANDLE g_GameLoopThread;

DWORD WINAPI GameLoop(LPVOID arg)
{
	HWND hWnd = (HWND)arg;

	g_GameScene = GameScene();
	g_OtherGameScene = OtherGameScene();
	for (std::queue<std::pair<int, eTileInfo>>& rBlockQueues : g_BlockQueues)
	{
		while (rBlockQueues.empty() == false)
		{
			rBlockQueues.pop();
		}
	}

	// 랜덤 초기화
	srand(time(NULL));

	g_Sock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_Sock == INVALID_SOCKET)
		err_quit("err_sock");

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	serveraddr.sin_port = htons(SERVERPORT);

	connect(g_Sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

	DWORD recvTimeout = 60000;  // 1분
	setsockopt(g_Sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&recvTimeout, sizeof(recvTimeout));


	FixedPacket recvBuffer(eNetworkPacketType::RecvPacket);
	int needLen = sizeof(recvBuffer);
	int recvLen = 0;
	while (1)
	{
		int recvRet = recv(g_Sock, (char*)(&recvBuffer) + recvLen, needLen, 0);
		if (recvRet == SOCKET_ERROR)
		{
			exit(EXIT_FAILURE);
			return -1;
		}

		recvLen += recvRet;
		needLen -= recvRet;
		if (recvLen == sizeof(recvBuffer))
		{
			break;
		}
	}


	if (recvBuffer.NetworkPacketType == eNetworkPacketType::InitGame)
	{
		char* recvBufDataPtr = recvBuffer.Data.data();

		g_ChannelNumber = recvBuffer.ChannelNumber;
		g_MainPlayerUserNumber = recvBuffer.UserNumber;

		assert((recvBuffer.UserNumber == 0) || (recvBuffer.UserNumber == 1));
		g_GameScene.mUserNumber = g_MainPlayerUserNumber;

		//other user size
		assert(*(uint32_t*)recvBufDataPtr == 1);
		recvBufDataPtr += sizeof(uint32_t);

		// other user num
		g_OtherGameScene.mUserNumber = *(uint32_t*)recvBufDataPtr;
		assert((g_OtherGameScene.mUserNumber == 0) || (g_OtherGameScene.mUserNumber == 1));
		recvBufDataPtr += sizeof(uint32_t);

		// block size
		const uint32_t blockSize = *(uint32_t*)recvBufDataPtr;
		recvBufDataPtr += sizeof(uint32_t);

		// user 0 block
		for (uint32_t i = 0; i < blockSize; ++i)
		{

			int type = *(int*)(recvBufDataPtr);
			recvBufDataPtr += sizeof(int);
			eTileInfo color = *(eTileInfo*)(recvBufDataPtr);
			recvBufDataPtr += sizeof(eTileInfo);

			g_BlockQueues[0].push(std::pair(type, color));
		}

		// user 1 block
		for (uint32_t i = 0; i < blockSize; ++i)
		{

			int type = *(int*)(recvBufDataPtr);
			recvBufDataPtr += sizeof(int);
			eTileInfo color = *(eTileInfo*)(recvBufDataPtr);
			recvBufDataPtr += sizeof(eTileInfo);

			g_BlockQueues[1].push(std::pair(type, color));
		}
	}

	recvTimeout = FRAME_TIME;  // 0.01초
	setsockopt(g_Sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&recvTimeout, sizeof(recvTimeout));

	state = Menu;
	g_GameScene.hWnd = hWnd;
	g_OtherGameScene.hWnd = hWnd;

	g_GameScene.Init(g_Sock);
	g_OtherGameScene.Init(g_Sock);

	state = Game;
	RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);

	auto start = std::chrono::high_resolution_clock::now();
	constexpr double fpsTime = 1.0 / 60.0;


	needLen = sizeof(recvBuffer);
	recvLen = 0;

	bool bRecved = false;

	while (state == Game)
	{
		if (static_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start).count() < fpsTime)
		{
			continue;
		}

		int recvRet = recv(g_Sock, (char*)(&recvBuffer) + recvLen, needLen, 0);
		if (recvRet != SOCKET_ERROR)
		{
			recvLen += recvRet;
			needLen -= recvRet;
			if (recvLen == sizeof(recvBuffer))
			{
				bRecved = true;

				needLen = sizeof(recvBuffer);
				recvLen = 0;
			}
		}
		else if (recvRet == 0)
		{
			state = End;
		}


		if (bRecved && recvBuffer.NetworkPacketType == eNetworkPacketType::ResultBlock)
		{
			char* recvBufDataPtr = recvBuffer.Data.data();

			int type = *(int*)(recvBufDataPtr);
			recvBufDataPtr += sizeof(int);
			eTileInfo color = *(eTileInfo*)(recvBufDataPtr);
			recvBufDataPtr += sizeof(eTileInfo);

			g_BlockQueues[recvBuffer.UserNumber].push(std::pair(type, color));

			recvBufDataPtr = nullptr;
		}

		g_GameScene.Input();
		g_GameScene.Update();

		if (bRecved)
		{
			g_OtherGameScene.Input(recvBuffer);
		}
		g_OtherGameScene.Update();

		// Render
		RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);

	}

	closesocket(g_Sock);
	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{

	//2조 추가


	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;



	printf("서버와 정상적으로 연결되었습니다.\n");
	//
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1600, 646,
		NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	g_GameLoopThread = CreateThread(NULL, 0, GameLoop, hWnd, 0, NULL);
	if (g_GameLoopThread == NULL)
	{
		return 1;
	}

	while (true)
	{
		if (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
		{
			if (Message.message == WM_QUIT || Message.message == WM_DESTROY)
			{
				break;
			}

			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}

	state = End;
	DWORD lpExitCode;
	if (GetExitCodeThread(g_GameLoopThread, &lpExitCode) == FALSE)
	{
		exit(0);
	}
	if (TerminateThread(g_GameLoopThread, lpExitCode) == FALSE)
	{
		exit(0);
	}
	g_GameLoopThread = nullptr;

	return Message.wParam;
}

void DrawBG(HINSTANCE hInst, HDC hdc, int id)
{
	HDC MemDC;
	HBITMAP MyBitmap, OldBitmap;
	MemDC = CreateCompatibleDC(hdc);
	MyBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(id));
	OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);

	// 2조 추가
	RECT rect = { 0, };
	rect.right = 1600;
	rect.bottom = 600;
	FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

	BitBlt(hdc, 0, 0, 800, 600, MemDC, 0, 0, SRCCOPY);
	SelectObject(MemDC, OldBitmap);
	DeleteObject(MyBitmap);
	DeleteDC(MemDC);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;

	g_GameScene.hInst = g_hInst;
	g_OtherGameScene.hInst = g_hInst;
	switch (iMessage) {
	case WM_CREATE:
		return 0;

	case WM_KEYDOWN:
		switch (state)
		{
		case Menu:
			switch (wParam)
			{
				// 게임으로 들어가기
			case VK_RETURN:
				/*
				gameScene.Init(g_Sock);
				otherGameScene.Init(g_Sock);
				state = Game;
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
				*/
				break;

			default:
				break;
			}
			break;

		case End:
			switch (wParam)
			{
			case VK_ESCAPE:
				g_GameScene.Exit();
				g_OtherGameScene.Exit();
				state = Menu;
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);

				DWORD lpExitCode;
				GetExitCodeThread(g_GameLoopThread, &lpExitCode);
				TerminateThread(g_GameLoopThread, lpExitCode);
				g_GameLoopThread = nullptr;

				g_GameLoopThread = CreateThread(NULL, 0, GameLoop, hWnd, 0, NULL);
				if (g_GameLoopThread == NULL)
				{
					return 1;
				}

				break;
			}
			break;
		}
		return 0;

	case WM_PAINT:
		g_GameScene.hdc = hdc = BeginPaint(hWnd, &ps);
		g_OtherGameScene.hdc = hdc;
		switch (state)
		{
		case Menu:
			DrawBG(g_hInst, hdc, IDB_MENUBG);
			break;
		case Game:
			g_GameScene.Render();
			g_OtherGameScene.Render();
			break;
		}
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		state = End;
		DWORD lpExitCode;
		if (GetExitCodeThread(g_GameLoopThread, &lpExitCode) == FALSE)
		{
			exit(0);
		}
		if (TerminateThread(g_GameLoopThread, lpExitCode) == FALSE)
		{
			exit(0);
		}
		g_GameLoopThread = nullptr;

		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

void err_quit(const char* message) {
	LPVOID IpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&IpMsgBuf,
		0, NULL
	);
	MessageBox(NULL, (LPCTSTR)IpMsgBuf, message, MB_ICONERROR);
	LocalFree(IpMsgBuf);
	exit(1);

}

void err_display(const char* message) {
	LPVOID IpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&IpMsgBuf,
		0, NULL
	);
	MessageBox(NULL, (LPCTSTR)IpMsgBuf, message, MB_ICONERROR);
	LocalFree(IpMsgBuf);
}

