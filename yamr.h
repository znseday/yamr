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

//class Reducer
//{
//private:
//    static size_t Number; // = 0;
//    size_t n;
//    std::vector<std::string> Res;

//public:
//    Reducer()
//    {
//        n = ++Number;
//    }

//    void operator()(const std::string &s)
//    {
//        Res.emplace_back(s);
//    }

//    ~Reducer()
//    {
//        std::stringstream ss;
//        ss << n << ".txt";
//        std::ofstream f(ss.str());
//        for (const auto &s : Res)
//            f << s << std::endl;
//        f.close();
//    }
//};


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

    std::vector<std::vector<std::string>> SectionsData;

    std::vector<std::vector<std::string>> DataForReducers;

    std::atomic_size_t FileNumber = 0;

public:
    Yamr(const std::string _fn, int _M, int _N);

    void Split();

    void Map(/*const*/ mr_type &); // нужен ли const?

    //void Shuffle();

    void Reduce(/*const*/ mr_type &); // нужен ли const?
};







