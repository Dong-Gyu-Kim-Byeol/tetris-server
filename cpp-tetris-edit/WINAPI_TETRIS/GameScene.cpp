#define _CRT_SECURE_NO_WARNINGS // 최신 VC++ 컴파일 시 경고 방지
#include "GameScene.h"

#include<WinSock2.h>

#include <array>
#include <cassert>
#include <queue>
#include <iostream>

#include "error.h"
#include "network.h"

// 2조 추가
uint32_t g_ChannelNumber;
uint32_t g_MainPlayerUserNumber;

SOCKET g_Sock;

std::array<std::queue<std::pair<int, eTileInfo>>, 2u> g_BlockQueues;

GameState state;

// 2조 추가
GameScene::GameScene() :
	mInputTime(GetTickCount())
{
}


GameScene::~GameScene()
{
}

void GameScene::Init(SOCKET sock)
{
	// 사운드 매니저 설정
	/*
	if (theSoundManager == nullptr)
	{
		theSoundManager = new SoundManager();

		theSoundManager->AddBGM("bgm.mp3");
		theSoundManager->AddSFX("block_down.mp3", "blockDown");
		theSoundManager->AddSFX("line_clear.mp3", "lineClear");
	}

	theSoundManager->PlayBGM();
	*/
	mSock = sock;

	ZeroMemory(mMap, HEIGHT * WIDTH);
	mCurBlock.Init();
	mNextBlock.Init();

	mHighScore = 0;
	mCurrentScore = 0;

	mLastTick = GetTickCount();

	mGuidePosition.x = WIDTH / 2;
	mGuidePosition.y = 0;

	mTurnTime = 0;
	mMoveTime = 0;
	mSpaceTime = 0;

	mScore = 0;

	mLose = false;

	mGuidePosition = { -99,-99 };

	inputType = eInputType::NONE;

	int s = g_BlockQueues[mUserNumber].size() - 1;
	std::pair<int, eTileInfo> block = g_BlockQueues[mUserNumber].front();
	g_BlockQueues[mUserNumber].pop();
	assert(s == g_BlockQueues[mUserNumber].size());

	FixedPacket sendBuf(eNetworkPacketType::NeedNewBlock, g_ChannelNumber, mUserNumber);
	send(mSock, (char*)(&sendBuf), sizeof(sendBuf), 0);

	mNextBlock.CreateBlock(block.first, block.second);
	ResetBlock();
}

void GameScene::Exit()
{
	//theSoundManager->Stop();
}

//2조 추가 : 키 입력시 서버로 키 입력값 전송
void GameScene::Input()
{

	int time = GetTickCount();
	if (mInputTime + FRAME_TIME > time)
	{
		return;
	}
	mInputTime = time;


	int retval;
	char err_send[] = "send()";
	// 좌우 이동
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		inputType = eInputType::LEFT;
	}

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		inputType = eInputType::RIGHT;
	}

	// 모양 바꾸기
	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		inputType = eInputType::UP;
	}

	// 아래로 내리기
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		inputType = eInputType::DOWN;
	}

	// 밑으로 쭉 내리기
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		inputType = eInputType::SPACE;
	}

	// 메뉴로 복귀
	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
	{
		inputType = eInputType::ESC;
	}

	//2조 추가
	assert(g_MainPlayerUserNumber == mUserNumber);
	FixedPacket sendBuf(eNetworkPacketType::InputKey, g_ChannelNumber, mUserNumber);
	char* pSendBufDataPtr = sendBuf.Data.data();
	static_assert(sizeof(char) == sizeof(uint8_t));

	// key
	*(eInputType*)(pSendBufDataPtr) = inputType;
	pSendBufDataPtr += sizeof(inputType);

	// color
	*(eTileInfo*)(pSendBufDataPtr) = mCurBlock.GetBlockColor();
	pSendBufDataPtr += sizeof(eTileInfo);

	// Position
	*(Point*)(pSendBufDataPtr) = mCurBlock.GetPosition();
	pSendBufDataPtr += sizeof(Point);

	assert((pSendBufDataPtr - sendBuf.Data.data()) <= sendBuf.Data.size());
	send(mSock, (char*)(&sendBuf), sizeof(sendBuf), 0); //키전송

	pSendBufDataPtr = nullptr;

}

void GameScene::Update()
{
	InputProcess();

	if (mLose) //패배했을때
	{
		return;
	}

	// 가이드블럭 위치 설정
	{
		Point point = mCurBlock.GetPosition();
		while (!CollisionCheck(point, mCurBlock))
		{
			point.y++;
		}
		point.y--;
		mGuidePosition = point;
	}

	//블럭 내려감, 블럭 제거 등등 기능 수행
	int curTick = GetTickCount();
	if (mLastTick + 500 < curTick)
	{
		mLastTick = curTick;

		//현재 컨트롤중인 블럭과 맵간의 충돌체크
		//좌표 한칸 내렸을때 충돌이 되면 끝, 아니면 한칸내림
		Point checkPosition = mCurBlock.GetPosition();
		checkPosition.y += 1;

		if (CollisionCheck(checkPosition, mCurBlock))
		{
			PutBlock();
			LineClear();
			ResetBlock();
		}
		else
		{
			mCurBlock.SetPosition(checkPosition);
		}
	}

}

void GameScene::Render()
{
	// 더블버퍼링용
	hdc2 = CreateCompatibleDC(hdc);
	newBitmap = CreateCompatibleBitmap(hdc, 800, 600);
	oldBitmap = (HBITMAP)SelectObject(hdc2, newBitmap);

	// 폰트 설정
	newFont = CreateFont(30, 0, 0, 0, 0, 0, 0, 0, HANGEUL_CHARSET, 0, 0, 0, VARIABLE_PITCH | FF_ROMAN, TEXT("배달의민족 주아"));
	oldFont = (HFONT)SelectObject(hdc2, newFont);

	// 1. 배경 그려줌
	DrawBG();

	// 2. 로고 출력
	DrawLogo();

	// 3. 쌓인블럭 그려줌
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			if (mMap[y][x] != EMPTY)
			{
				DrawBox(Point{ x, y }, mMap[y][x]);
			}
		}
	}

	// 4. 다음블럭 그려줌
	DrawBlock(mNextBlock, Point{ 13, 2 }, mNextBlock.GetBlockColor());

	if (!mLose)
	{
		// 5. 가이드블럭 그려줌
		if (mGuidePosition.x != -99)
			DrawBlock(mCurBlock, mGuidePosition, eTileInfo::GUIDE);

		// 6. 내 블럭 그려줌
		DrawBlock(mCurBlock, mCurBlock.GetPosition(), mCurBlock.GetBlockColor());
	}

	//스코어 표시해줌
	char score[100] = { 0 };
	sprintf(score, "SCORE: %5d", mScore);
	TextOut(hdc2, 420, 20, score, strlen(score));

	// 게임오버일 경우 게임오버 판넬 출력
	if (mLose)
	{
		state = GameState::End;
		DrawGameOver();
	}

	// 더블버퍼링
	// 2조 추가
	int startPointX = 0;
	if (mUserNumber != g_MainPlayerUserNumber)
	{
		startPointX = 800;
	}
	BitBlt(hdc, startPointX, 0, 800, 600, hdc2, 0, 0, SRCCOPY);

	SelectObject(hdc2, oldFont);
	DeleteObject(newFont);

	SelectObject(hdc2, oldBitmap);
	DeleteObject(newBitmap);
	DeleteDC(hdc2);
}

void GameScene::ResetBlock()
{
	mCurBlock.CopyBlock(mNextBlock);

	std::pair<int, eTileInfo> block = g_BlockQueues[mUserNumber].front();
	g_BlockQueues[mUserNumber].pop();

	mNextBlock.CreateBlock(block.first, block.second);

	FixedPacket sendBuf(eNetworkPacketType::NeedNewBlock, g_ChannelNumber, mUserNumber);
	send(mSock, (char*)(&sendBuf), sizeof(sendBuf), 0);


	mCurBlock.SetPosition(Point{ WIDTH / 2,0 });

	if (mCurBlock.GetBlockType() >= 5)
	{
		Point normBlockPosition = mCurBlock.GetPosition() + Point{ 0,-1 };
		mCurBlock.SetPosition(normBlockPosition);
	}

	while (CollisionCheck(mCurBlock.GetPosition(), mCurBlock))
	{
		mCurBlock.SetPosition(mCurBlock.GetPosition() + Point{ 0,-1 });
	}
}

//2조 추가 : 서버로부터 데이터 받고 데이터 인풋 
void GameScene::InputProcess()
{


	if (mLose)
	{
		if (inputType == eInputType::ESC)
			mbEscape = true;
		return;
	}

	Point checkPosition = mCurBlock.GetPosition();
	int time = GetTickCount();

	switch (inputType)
	{
	case eInputType::NONE: // 아무것도 안한다.
		break;

	case eInputType::LEFT: // 왼쪽으로 이동



		if (mMoveTime + MOVE_DELAY < time)
		{
			mMoveTime = time;
			checkPosition.x -= 1;
			if (!CollisionCheck(checkPosition, mCurBlock))
			{
				//충돌안하면 변경
				mCurBlock.SetPosition(checkPosition);
			}
		}
		break;

	case eInputType::RIGHT: // 오른쪽으로 이동
		if (mMoveTime + MOVE_DELAY < time)
		{
			mMoveTime = time;
			checkPosition.x += 1;
			if (!CollisionCheck(checkPosition, mCurBlock))
			{
				//충돌안하면 변경
				mCurBlock.SetPosition(checkPosition);
			}
		}
		break;

	case eInputType::UP: // 모양바꾸는것
		if (mTurnTime + TURN_DELAY < time)
		{
			mTurnTime = time;
			TurnBlock();
		}
		break;

	case eInputType::DOWN: // 아래로 한칸 내림
		if (mMoveTime + MOVE_DELAY < time)
		{
			mMoveTime = time;
			checkPosition.y += 1;
			if (!CollisionCheck(checkPosition, mCurBlock))
			{
				//충돌안하면 변경
				mCurBlock.SetPosition(checkPosition);
			}
		}
		break;

	case eInputType::SPACE: // 밑으로 쭉 내림
		if (mSpaceTime + SPACE_DELAY < time)
		{
			mSpaceTime = time;
			while (!CollisionCheck(checkPosition, mCurBlock))
			{
				checkPosition.y++;
			}
			checkPosition.y--;

			mCurBlock.SetPosition(checkPosition);
			mLastTick -= 600;
		}
		break;

	case eInputType::ESC:
		mbEscape = true;
		break;

	default:
		assert(0);
		break;
	}

	// 키입력 처리 후 똑같은거 수행안되게 입력을 없앰
	inputType = eInputType::NONE;
}

bool GameScene::CollisionCheck(Point nextPosition, Block& block)
{
	// 블록 각각의 위치를 구해준다.
	Point blockPosition[4];
	block.GetEachTilePosition(nextPosition, blockPosition);

	// 마지막블록의 절대위치가 0보다작다면(게임판 꼭대기보다 위에 위치한다면)
	// 패배로 처리한다.
	if (blockPosition[3].y < 0)
	{
		mLose = true;
		return false;
	}

	for (int i = 0; i < 4; i++)
	{
		// 블록이 맵밖으로 나가거나 블록의 위치에 다른블록이 있다면 충돌
		if (blockPosition[i].x < 0 || blockPosition[i].x >= WIDTH ||
			blockPosition[i].y > HEIGHT - 1)
			return true;

		if (blockPosition[i].y >= 0 && blockPosition[i].x >= 0 &&
			mMap[blockPosition[i].y][blockPosition[i].x] != eTileInfo::EMPTY)
		{
			return true;
		}
	}

	return false;
}

void GameScene::PutBlock()
{
	Point blockPosition[4];
	eTileInfo color = mCurBlock.GetBlockColor();
	mCurBlock.GetEachTilePosition(mCurBlock.GetPosition(), blockPosition);

	for (int i = 0; i < 4; i++)
	{
		if (blockPosition[i].y >= 0)
			mMap[blockPosition[i].y][blockPosition[i].x] = color;
	}

	mGuidePosition = { -99,-99 };

	//theSoundManager->PlaySFX("blockDown");
}

void GameScene::LineClear()
{
	unsigned char zero[WIDTH] = { EMPTY };

	// 한줄로 꽉찬 라인을 전부 비워준다.
	for (int y = HEIGHT - 1; y >= 0; y--)
	{
		// 블록한줄이 꽉차있는지 확인
		bool bFilled = true;
		for (int x = 0; x < WIDTH; x++)
		{
			if (mMap[y][x] == EMPTY)
			{
				bFilled = false;
				break;
			}
		}

		// 꽉차있다면 점수늘리고 블록을 비워줌
		if (bFilled)
		{
			mScore += 10;
			memset(mMap[y], EMPTY, WIDTH);
			//theSoundManager->PlaySFX("lineClear");
		}
	}

	// 빈 라인이 있으면 윗라인을 당겨온다.
	for (int y = HEIGHT - 1; y >= 0; y--)
	{
		// y번 줄이 비어있는지 확인
		if (memcmp(mMap[y], zero, WIDTH) == 0)
		{
			// 위로 올라가면서 안비어있는줄 찾는다(index에 안비어있는곳 저장됨)
			int index = y;
			while (memcmp(mMap[index], zero, WIDTH) == 0)
			{
				index--;
				if (index < 0)
					break;
			}

			// 안비어있는 줄을 찾았다면 해당줄의 내용을 y번째 줄로 옮기고, index번 줄을 공백으로 채운다.
			if (index >= 0)
			{
				memcpy(mMap[y], mMap[index], WIDTH);
				ZeroMemory(mMap[index], WIDTH);
			}
		}
	}
}

void GameScene::TurnBlock()
{
	Point blockPosition[4];
	Point position;

	int type = 0;
	int color = 0;

	Block tempBlock;

	mCurBlock.GetEachTileVecPosition(blockPosition);
	position = mCurBlock.GetPosition();

	type = mCurBlock.GetBlockType();
	color = mCurBlock.GetBlockColor();

	tempBlock.CopyBlock(mCurBlock);

	if (type < 5)
	{
		for (int i = 0; i < 4; i++)
		{
			Point turnPoint;
			turnPoint.x = blockPosition[i].y;
			turnPoint.y = blockPosition[i].x;

			switch (turnPoint.x)
			{
			case 0:
				turnPoint.x = 2;
				break;
			case 2:
				turnPoint.x = 0;
				break;
			}

			blockPosition[i].x = turnPoint.x;
			blockPosition[i].y = turnPoint.y;
		}
	}
	else if (type == 6)
	{
		for (int i = 0; i < 4; i++)
		{
			unsigned char temp = blockPosition[i].x;
			blockPosition[i].x = blockPosition[i].y;
			blockPosition[i].y = temp;
		}
	}

	tempBlock.SetEachTilePosition(blockPosition);

	if (CollisionCheck(tempBlock.GetPosition(), tempBlock))
	{
		for (int i = 1; i < 4; i++)
		{
			if (!CollisionCheck(position + Point{ -i,0 }, tempBlock))
			{
				position.x -= i;
				mCurBlock.CopyBlock(tempBlock);
				mCurBlock.SetPosition(position);
				return;
			}
		}

		for (int i = 1; i < 4; i++)
		{
			if (!CollisionCheck(position + Point{ i,0 }, tempBlock))
			{
				position.x += i;
				mCurBlock.CopyBlock(tempBlock);
				mCurBlock.SetPosition(position);
				return;
			}
		}
	}
	else
	{
		mCurBlock.CopyBlock(tempBlock);
		mCurBlock.SetPosition(position);
	}

}

void GameScene::DrawBG()
{
	HDC MemDC;
	HBITMAP MyBitmap, OldBitmap;
	MemDC = CreateCompatibleDC(hdc);
	MyBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GAMEBG));
	OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);

	BitBlt(hdc2, 0, 0, 800, 600, MemDC, 0, 0, SRCCOPY);

	SelectObject(MemDC, OldBitmap);
	DeleteObject(MyBitmap);
	DeleteDC(MemDC);
}

void GameScene::DrawBlock(Block& block, Point position, eTileInfo color)
{
	HDC MemDC;
	HBITMAP MyBitmap, OldBitmap;
	MemDC = CreateCompatibleDC(hdc);
	MyBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(color));
	OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);

	Point blockPosition[4];
	block.GetEachTilePosition(position, blockPosition);

	for (int i = 0; i < 4; i++)
	{
		BitBlt(hdc2, blockPosition[i].x * 40, blockPosition[i].y * 40, blockPosition[i].x * 40 + 40, blockPosition[i].y * 40 + 40, MemDC, 0, 0, SRCCOPY);
	}

	SelectObject(MemDC, OldBitmap);
	DeleteObject(MyBitmap);
	DeleteDC(MemDC);
}

void GameScene::DrawBox(Point position, int color)
{
	HDC MemDC;
	HBITMAP MyBitmap, OldBitmap;
	MemDC = CreateCompatibleDC(hdc);
	MyBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(color));
	OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);

	BitBlt(hdc2, position.x * 40, position.y * 40, position.x * 40 + 40, position.y * 40 + 40, MemDC, 0, 0, SRCCOPY);

	SelectObject(MemDC, OldBitmap);
	DeleteObject(MyBitmap);
	DeleteDC(MemDC);
}

void GameScene::DrawGameOver()
{
	HDC MemDC;
	HBITMAP MyBitmap, OldBitmap;
	MemDC = CreateCompatibleDC(hdc);
	MyBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GameOver));
	OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);

	int halfX = SCREEN_WIDTH / 2;
	int halfY = SCREEN_HEIGHT / 2;

	BITMAP bitmap;
	::GetObject(MyBitmap, sizeof(BITMAP), (LPVOID)&bitmap);

	int bitmapWidth = bitmap.bmWidth;
	int bitmapHeight = bitmap.bmHeight;

	TransparentBlt(hdc2, halfX - bitmapWidth / 2, halfY - bitmapHeight / 2, bitmapWidth, bitmapHeight, MemDC, 0, 0, bitmapWidth, bitmapHeight, RGB(255, 0, 255));
	//BitBlt(hdc2, halfX - bitmapWidth / 2, halfY - bitmapHeight / 2, bitmapWidth, bitmapHeight, MemDC, 0, 0, SRCCOPY);

	SelectObject(MemDC, OldBitmap);
	DeleteObject(MyBitmap);
	DeleteDC(MemDC);
}

void GameScene::DrawLogo()
{
	HDC MemDC;
	HBITMAP MyBitmap, OldBitmap;
	MemDC = CreateCompatibleDC(hdc);
	MyBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_Logo));
	OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);

	int halfX = SCREEN_WIDTH / 2;
	int halfY = SCREEN_HEIGHT / 2;

	BitBlt(hdc2, halfX + 20, 230, 350, 350, MemDC, 0, 0, SRCCOPY);

	SelectObject(MemDC, OldBitmap);
	DeleteObject(MyBitmap);
	DeleteDC(MemDC);
}


void OtherGameScene::Input(const FixedPacket& rRecvPacket)
{
	if (rRecvPacket.UserNumber != mUserNumber)
	{
		return;
	}

	if (rRecvPacket.NetworkPacketType != eNetworkPacketType::InputKey)
	{
		return;
	}

	int time = GetTickCount();
	if (mInputTime + FRAME_TIME > time)
	{
		return;
	}
	mInputTime = time;

	const char* pRecvPacketDataPtr = rRecvPacket.Data.data();

	// key
	inputType = *(eInputType*)(pRecvPacketDataPtr);
	assert((uint32_t)inputType <= eInputType::ESC);
	pRecvPacketDataPtr += sizeof(eInputType);

	// color
	if (mCurBlock.GetBlockColor() == *(eTileInfo*)(pRecvPacketDataPtr))
	{
		pRecvPacketDataPtr += sizeof(eTileInfo);

		// position
		mCurBlock.SetPosition(*(Point*)(pRecvPacketDataPtr));
		pRecvPacketDataPtr += sizeof(Point);
	}
}