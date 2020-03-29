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
#include <queue>
#include <thread>
#include <future>
#include <atomic>


// __FUNCSIG__ is for VS, but Qt (mingw) works with __PRETTY_FUNCTION__
#if ((defined WIN32) || (defined WIN64)) && (defined _MSC_VER)
//#define MY_P_FUNC __FUNCSIG__
#else
#define MY_P_FUNC __PRETTY_FUNCTION__
#endif

extern bool IsDebugOutput;

#define MY_DEBUG_ONLY(x) { if(IsDebugOutput) {x}  }

//-----------------------------------------------


//using reducer_type = decltype([](const std::string &)->std::vector<std::string>{return std::vector<std::string>{};});

using mr_type = std::function<std::vector<std::string>(const std::string &)>;

class Yamr
{
private:
    std::string fn;
    size_t M;
    size_t R;

    std::mutex m_console;

    std::vector<std::size_t> SectionsInfo;

    std::atomic_size_t FileNumber = 0;

public:
    Yamr(const std::string _fn, int _M, int _N);

    void Split();

    void Map(const mr_type &);

    void Reduce(const mr_type &);
};







