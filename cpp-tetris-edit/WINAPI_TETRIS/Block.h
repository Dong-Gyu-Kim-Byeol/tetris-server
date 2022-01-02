#pragma once
#include "헤더.h"
#define COLOR_COUNT 6

struct Point
{
	int x;
	int y;

	Point operator += (Point another)
	{
		x += another.x;
		y += another.y;
		return *this;
	}

	Point operator + (Point another)
	{
		return Point{ x + another.x,y += another.y };
	}
};

enum eTileInfo : uint32_t
{ 
	EMPTY = 0, RED = 101, ORANGE, YELLOW, GREEN, BLUE, PURPLE, GRAY = 198, GUIDE = 199 
};


class Block
{
private:
	eTileInfo mTileData[4][4];// 블록정보 저장
	Point mPosition;//위치정보
	eTileInfo mColor; //컬러정보
	int mType;  //블록의 타입

public:
	void Init();
	void CreateBlock(const int type, const eTileInfo color);
	void CopyBlock(Block& block);

	void GetEachTilePosition(Point nextPosition,Point* outPoint);	// 맵상의 타일 각각의 위치를 구해준다.
	void GetEachTileVecPosition(Point* outPoint);	// 타일 각각의 상대위치를 구해준다.
	void SetEachTilePosition(Point inPosition[4]);

	Point GetPosition();
	void SetPosition(Point position);

	void GetBlockData(eTileInfo outData[4][4]);
	void SetBlockData(eTileInfo data[4][4]);

	int GetBlockType();
	void SetBlockType(int type);

	eTileInfo GetBlockColor();


public:
	Block();
	~Block();
};

