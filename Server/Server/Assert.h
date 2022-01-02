#pragma once
#include <iostream>

#define ASSERT_USE (0)

#define DEBUG_BREAK_EXIT															\
	__debugbreak();	/*__asm { int 3 };*/											\
	std::exit(EXIT_FAILURE);												

#if ASSERT_USE
#define Assert(condition, msg)													\
	if(!(condition))																\
	{																				\
		fprintf(stderr, "%s ( %s : %d )\n", (const char*)msg, __FILE__, __LINE__);	\
		DEBUG_BREAK_EXIT															\
	}					
#else
#define Assert(condition, msg) __assume(condition);
#endif
