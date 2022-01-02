#include "Block.h"

#include <assert.h>

Block::Block()
{
}


Block::~Block()
{
}

void Block::Init()
{
	ZeroMemory(mTileData, sizeof(eTileInfo) * 4 * 4);
	mPosition = { 0, 0 };
	mColor = eTileInfo::EMPTY;
	mType = 0;
}

// 2조 추가
void Block::CreateBlock(const int type, const eTileInfo color)
{
	assert((0 <= type) && (type <= 6));
	assert(eTileInfo::EMPTY != color);

	eTileInfo differentColor = color;

	if (mColor == color)
	{
		if (color == PURPLE)
		{
			differentColor = (eTileInfo)((uint32_t)differentColor - 1);
		}
		else
		{
			differentColor = (eTileInfo)((uint32_t)differentColor + 1);
		}
	}

	Init();
	
	mType = type;
	mColor = differentColor;

	switch (mType)
	{
	case 0:
		/*
		■■□
		□■■
		□□□	*/
		mTileData[0][0] = mColor;
		mTileData[0][1] = mColor;
		mTileData[1][1] = mColor;
		mTileData[1][2] = mColor;
		break;

	case 1:
		/*
		□■■
		■■□
		□□□	*/
		mTileData[0][1] = mColor;
		mTileData[0][2] = mColor;
		mTileData[1][0] = mColor;
		mTileData[1][1] = mColor;
		break;

	case 2:
		/*
		■■□
		□■□
		□■□	*/
		mTileData[0][0] = mColor;
		mTileData[0][1] = mColor;
		mTileData[1][1] = mColor;
		mTileData[2][1] = mColor;
		break;

	case 3:
		/*
		□■■
		□■□
		□■□	*/
		mTileData[0][1] = mColor;
		mTileData[0][2] = mColor;
		mTileData[1][1] = mColor;
		mTileData[2][1] = mColor;
		break;

	case 4:
		/*
		□■□
		■■■
		□□□	*/
		mTileData[0][1] = mColor;
		mTileData[1][0] = mColor;
		mTileData[1][1] = mColor;
		mTileData[1][2] = mColor;
		break;

	case 5:
		/*
		□□□□
		□■■□
		□■■□
		□□□□	*/
		mTileData[1][1] = mColor;
		mTileData[1][2] = mColor;
		mTileData[2][1] = mColor;
		mTileData[2][2] = mColor;
		break;

	case 6:
		/*
		□□□□
		■■■■
		□□□□
		□□□□	*/
		mTileData[1][0] = mColor;
		mTileData[1][1] = mColor;
		mTileData[1][2] = mColor;
		mTileData[1][3] = mColor;
		break;
	// 2조 추가
	default:
		assert(0);
	}
}

void Block::CopyBlock(Block& block)
{
	eTileInfo tempTiles[4][4];
	block.GetBlockData(tempTiles);
	SetBlockData(tempTiles);
	mType = block.GetBlockType();
	mColor = block.GetBlockColor();
	mPosition = block.GetPosition();
}

Point Block::GetPosition()
{
	return mPosition;
}

void Block::SetPosition(Point position)
{
	mPosition = position;
}

void Block::GetBlockData(eTileInfo outData[4][4])
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			outData[i][j] = mTileData[i][j];
		}
	}
}

void Block::SetBlockData(eTileInfo data[4][4])
{
	memcpy(mTileData, data, sizeof(eTileInfo) * 4 * 4);
}

int Block::GetBlockType()
{
	return mType;
}

void Block::SetBlockType(int type)
{
	mType = type;
}

eTileInfo Block::GetBlockColor()
{
	return mColor;
}

void Block::GetEachTilePosition(Point nextPosition, Point* outPoint)
{
	int pointerIndex = 0;

	for (int y = 0; y < 4; y++)
	{
		if (pointerIndex >= 4)
			break;
		for (int x = 0; x < 4; x++)
		{
			if (mTileData[y][x] != eTileInfo::EMPTY)
			{
				outPoint[pointerIndex].x = x;
				outPoint[pointerIndex].y = y;
				outPoint[pointerIndex] += nextPosition;
				pointerIndex++;
			}
		}
	}
}

void Block::GetEachTileVecPosition(Point* outPoint)
{
	GetEachTilePosition(Point{ 0,0 }, outPoint);
}

void Block::SetEachTilePosition(Point inPosition[4])
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			mTileData[i][j] = eTileInfo::EMPTY;
		}
	}

	for (int i = 0; i < 4; i++)
	{
		mTileData[inPosition[i].y][inPosition[i].x] = mColor;
	}
}