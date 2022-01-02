#pragma once
#include <array>
#include <queue>

#include "���.h"
#include "resource.h"
#include "Block.h"
//#include "SoundManager.h"

// 2�� �߰�
#include "network.h"


// 2�� �߰�
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

	// 2�� �߰�

	//int mType;
	//eTileInfo mColor;
	int mInputTime;

	unsigned char mMap[HEIGHT][WIDTH];	// ��ü ���� �����ϴ� ����
	Block mNextBlock;
	Block mCurBlock;

	Point mGuidePosition;		// ����� ���� �������� �����ִ� ���̵���� ��ġ

	int mHighScore;
	int mCurrentScore;

	int mLastTick;

	int mMoveTime;			// Ű�� ������ �󸶳� �������� ����
	int mTurnTime;
	int mSpaceTime;

	int mScore;

	bool mLose;		// ���ӿ� �������
	bool mbEscape;	// �޴�����

public:
	uint32_t mUserNumber;
	SOCKET mSock;

	HWND hWnd;
	HINSTANCE hInst;
	HDC hdc;
	HDC hdc2;
	HBITMAP newBitmap, oldBitmap;

	HFONT newFont, oldFont;

	eInputType inputType;	// �������� �Էµ� ��� ����

protected:
	void InputProcess();	// �ԷµȰ� ó��

	void ResetBlock();		// ���� ����� ����������� �ٲ��ְ�, ��������� ���� �����Ѵ�.
	bool CollisionCheck(Point nextPosition, Block& block);	// �ش���ġ�� ����� �����Ұ�� ���� ��ϰ� �浹�ϴ��� üũ
	void PutBlock();		// ����� ������ġ�� �д�.
	void LineClear();		// �ʿ� ���ٷ� ä���� ������ �����ش�.
	void TurnBlock();		// ���� ����� ȸ����Ŵ

	void DrawBG();		// ����� �׷��ش�.
	void DrawBlock(Block& block, Point position, eTileInfo color);		// ���� ��, ���� ��, ���̵���� �׷��ش�.
	void DrawBox(Point position, int type);		// ���κ���� ��ĭ��ĭ �׷��ش�.
	void DrawGameOver();	// ���ӿ��� �ǳ� ���
	void DrawLogo();	// �ΰ� ���



public:
	void Init(SOCKET sock);	// �ʱ�ȭ
	void Input();	// �Է�
	void Update();	// ������Ʈ
	void Render();	// ����

	void Exit(); //����

	GameScene();
	virtual ~GameScene();
};

class OtherGameScene : public GameScene
{
public:
	void Input(const FixedPacket& rRecvPacket);

};