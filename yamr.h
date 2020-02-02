#pragma once

#include <iostream>
#include <fstream>

#include <vector>
#include <iterator>
#include <algorithm>
#include <memory>
//#include <chrono>
//#include <ratio>
#include <thread>
#include <ctime>
#include <sstream>
#include <condition_variable>
#include <iostream>
#include <queue>
#include <thread>
#include <future>


// __FUNCSIG__ is for VS, but Qt (mingw) works with __PRETTY_FUNCTION__
#if ((defined WIN32) || (defined WIN64)) && (defined _MSC_VER)
//#define MY_P_FUNC __FUNCSIG__
#else
#define MY_P_FUNC __PRETTY_FUNCTION__
#endif

#if ((defined NDEBUG) || (defined _NDEBUG))
#define MY_DEBUG_ONLY(x)
#else
#define MY_DEBUG_ONLY(x) (x)
#endif

void TestFile(const char *file_name);
//-----------------------------------------------

extern std::mutex console_m;

//using args = std::tuple<
////        std::function<void(const std::string &, const std::string &)>,
//        std::future<void>
//        >;


//using args = std::function<void(CommandsType)>;

//using args = std::future<void(CommandBlocksType)>;

//using args = std::tuple<
//        std::function<void(void)>,
//        CommandBlocksType &
//        >;

//-----------------------------------------------





