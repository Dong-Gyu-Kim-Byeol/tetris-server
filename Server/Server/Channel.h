#pragma once

#include <array>
#include <iostream>

#include "Typedef.h"
#include "Assert.h"
#include "SocketInfo.h"

class User final
{
public:
	User() :
		mUserNumber(-1),
		mChannelNumber(-1)
	{
		Assert(this == (void*)&mRecvSocketInfo, "");
		Assert(&mRecvSocketInfo ==
			(SocketInfo*)((uint8*)(&mSendSocketInfo) - sizeof(mSendSocketInfo)), "");
		Assert(this ==
			(void*)((uint8*)(&mSendSocketInfo) - sizeof(mSendSocketInfo)), "");
	}

	User(const uint32 userNumber, const uint32 channelNumber, const SocketInfo& rRecvSocketInfo, const SocketInfo& rSendSocketInfo) :
		mUserNumber(userNumber),
		mChannelNumber(channelNumber),
		mRecvSocketInfo(rRecvSocketInfo),
		mSendSocketInfo(rSendSocketInfo)
	{
		Assert(this == (void*)&mRecvSocketInfo, "");
		Assert(&mRecvSocketInfo ==
			(SocketInfo*)((uint8*)(&mSendSocketInfo) - sizeof(mSendSocketInfo)), "");
		Assert(this ==
			(void*)((uint8*)(&mSendSocketInfo) - sizeof(mSendSocketInfo)), "");
		Assert(mRecvSocketInfo.mSock == mSendSocketInfo.mSock, "");

	}

	void Init(const uint32 userNumber, const uint32 channelNumber, const SocketInfo& rRecvSocketInfo, const SocketInfo& rSendSocketInfo)
	{
		Assert(this == (void*)&mRecvSocketInfo, "");
		Assert(&mRecvSocketInfo ==
			(SocketInfo*)((uint8*)(&mSendSocketInfo) - sizeof(mSendSocketInfo)), "");
		Assert(this ==
			(void*)((uint8*)(&mSendSocketInfo) - sizeof(mSendSocketInfo)), "");

		mUserNumber = userNumber;
		mChannelNumber = channelNumber;
		mRecvSocketInfo = rRecvSocketInfo;
		mSendSocketInfo = rSendSocketInfo;

		Assert(mRecvSocketInfo.mSock == mSendSocketInfo.mSock, "");
	}

	~User() = default;

	SocketInfo& GetRecvSocketInfo()
	{
		return mRecvSocketInfo;
	}

	SocketInfo& GetSendSocketInfo()
	{
		return mSendSocketInfo;
	}

	uint32 GetChannelNumber() const
	{
		return mChannelNumber;
	}

	uint32 GetUserNumber() const
	{
		return mUserNumber;
	}

	static User* GetUserPointer(const SocketInfo& rSocketInfo)
	{
		if (rSocketInfo.mbRecvInfo == true)
		{
			return (User*)&rSocketInfo;
		}
		else
		{
			return (User*)((uint8*)(&rSocketInfo) - sizeof(rSocketInfo));
		}
	}

private:
	SocketInfo mRecvSocketInfo;
	SocketInfo mSendSocketInfo;
	uint32 mChannelNumber;
	uint32 mUserNumber;
};

class Channel final
{
public:
	enum
	{
		USER_SIZE = 2u
	};

	Channel() :
		mChannelNumber(-1)
	{
		static_assert(USER_SIZE != -1);
	}

	Channel(const uint32 channelNumber,
		const SocketInfo& rUserRecvSocketInfo_0, const SocketInfo& rUserSendSocketInfo_0,
		const SocketInfo& rUserRecvSocketInfo_1, const SocketInfo& rUserSendSocketInfo_1) :
		mChannelNumber(channelNumber),
		mUsers({
			User(0, channelNumber , rUserRecvSocketInfo_0, rUserSendSocketInfo_0),
			User(1, channelNumber , rUserRecvSocketInfo_1, rUserSendSocketInfo_1)
			})
	{
		static_assert(USER_SIZE != -1);
		Assert(channelNumber != -1, "");
	}

	void Init(const uint32 channelNumber,
		const SocketInfo& rUserRecvSocketInfo_0, const SocketInfo& rUserSendSocketInfo_0,
		const SocketInfo& rUserRecvSocketInfo_1, const SocketInfo& rUserSendSocketInfo_1)
	{
		static_assert(USER_SIZE != -1);
		Assert(channelNumber != -1, "");

		mChannelNumber = channelNumber;
		mUsers = { User(0, channelNumber , rUserRecvSocketInfo_0, rUserSendSocketInfo_0),
			User(1, channelNumber , rUserRecvSocketInfo_1, rUserSendSocketInfo_1) };
	}

	Channel(const uint32 channelNumber, User& rUser_0, User& rUser_1) :
		mChannelNumber(channelNumber),
		mUsers({ rUser_0, rUser_1 })
	{
		Assert(channelNumber != -1, "");
		Assert(rUser_0.GetUserNumber() != -1, "");
		Assert(rUser_1.GetUserNumber() != -1, "");
	}

	void Init(const uint32 channelNumber, User& rUser_0, User& rUser_1)
	{
		Assert(channelNumber != -1, "");
		Assert(rUser_0.GetUserNumber() != -1, "");
		Assert(rUser_1.GetUserNumber() != -1, "");

		mChannelNumber = channelNumber;
		mUsers = { rUser_0, rUser_1 };
	}

	void Delete()
	{
		for (uint32 userNumber = 0; userNumber < Channel::USER_SIZE; ++userNumber)
		{
			closesocket(mUsers[userNumber].GetRecvSocketInfo().mSock);
		}
	}

	~Channel() = default;

	/*
	void SetChannelNumber(const uint32 channelNumber)
	{
		mChannelNumber = channelNumber;
	}

	void SetUser(const uint32 userNumber, const User socketInfo)
	{
		Assert(userNumber < USER_SIZE, "");
		mUsers[userNumber] = socketInfo;
	}
	*/
	uint32 GetChannelNumber()
	{
		return mChannelNumber;
	}

	User& GetUser(const uint32 userNumber)
	{
		Assert(userNumber < USER_SIZE, "");
		return mUsers[userNumber];
	}

private:
	uint32 mChannelNumber;
	std::array<User, USER_SIZE> mUsers;
};