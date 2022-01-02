#pragma once
#include <array>
#include <queue>

#include "헤더.h"
#include "resource.h"
#include "Block.h"
//#include "SoundManager.h"

// 2조 추가
#include "network.h"


// 2조 추가
constexpr DWORD FRAME_TIME = 1000 / 120;

extern uint32_t g_ChannelNumber;
extern uint32_t g_MainPlayerUserNumber;

extern std::array<std::queue<std::pair<int, eTileInfo>>, 2u> g_BlockQueues;

extern SOCKET g_Sock;

enum GameState { Menu, Game, End };
extern GameState state;

const int MOVE_DELAY = 100;
const int TURN_DELAY = 150;
const int SPACE_DELAY = 150;

enum eInputType : uint32_t
{
	NONE, LEFT, RIGHT, UP, DOWN, SPACE, ESC
};

class GameScene
{
protected:
	//SoundManager* theSoundManager;

	// 2조 추가

	//int mType;
	//eTileInfo mColor;
	int mInputTime;

	unsigned char mMap[HEIGHT][WIDTH];	// 전체 맵을 저장하는 공간
	Block mNextBlock;
	Block mCurBlock;

	Point mGuidePosition;		// 블록이 어디로 떨어질지 보여주는 가이드라인 위치

	int mHighScore;
	int mCurrentScore;

	int mLastTick;

	int mMoveTime;			// 키를 누르고 얼마나 지났는지 측정
	int mTurnTime;
	int mSpaceTime;

	int mScore;

	bool mLose;		// 게임에 졌을경우
	bool mbEscape;	// 메뉴복귀

public:
	uint32_t mUserNumber;
	SOCKET mSock;

	HWND hWnd;
	HINSTANCE hInst;
	HDC hdc;
	HDC hdc2;
	HBITMAP newBitmap, oldBitmap;

	HFONT newFont, oldFont;

	eInputType inputType;	// 마지막에 입력된 명령 저장

protected:
	void InputProcess();	// 입력된것 처리

	void ResetBlock();		// 현재 블록을 다음블록으로 바꿔주고, 다음블록을 새로 생성한다.
	bool CollisionCheck(Point nextPosition, Block& block);	// 해당위치에 블록이 존재할경우 기존 블록과 충돌하는지 체크
	void PutBlock();		// 블록을 현재위치에 둔다.
	void LineClear();		// 맵에 한줄로 채워진 라인을 지워준다.
	void TurnBlock();		// 현재 블록을 회전시킴

	void DrawBG();		// 배경을 그려준다.
	void DrawBlock(Block& block, Point position, eTileInfo color);		// 현재 블럭, 다음 블럭, 가이드블럭을 그려준다.
	void DrawBox(Point position, int type);		// 쌓인블록을 한칸한칸 그려준다.
	void DrawGameOver();	// 게임오버 판넬 출력
	void DrawLogo();	// 로고 출력



public:
	void Init(SOCKET sock);	// 초기화
	void Input();	// 입력
	void Update();	// 업데이트
	void Render();	// 렌더

	void Exit(); //종료

	GameScene();
	virtual ~GameScene();
};

class OtherGameScene : public GameScene
{
public:
	void Input(const FixedPacket& rRecvPacket);

};