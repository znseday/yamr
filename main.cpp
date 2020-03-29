#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cassert>
#include <thread>
#include <cstring>

#include "yamr.h"

using namespace std;

constexpr int MAX_THREADS = 1000;

int main(int argc, const char **argv)
{
    if (argc < 4)
    {
        cout << "My error: there must be 3 parametrs: <src> <mnum> <rnum>" << endl;
        exit(0);
    }

    const string fn = argv[1];
    const int M = atoi(argv[2]);
    const int R = atoi(argv[3]);

    if (fn.empty() || M < 1 || R < 1 || M > MAX_THREADS || R > MAX_THREADS)
    {
        cout << "My error: something's wrong with arguments" << endl;
        exit(0);
    }

    if (argc > 4 && ( strcmp(argv[4], "-d") == 0 || strcmp(argv[4], "d") == 0) )
        IsDebugOutput = true;

    MY_DEBUG_ONLY(cout << "fn = " << fn << endl;)
    MY_DEBUG_ONLY(cout << "M = "  << M << endl;)
    MY_DEBUG_ONLY(cout << "R = "  << R << endl;)

    Yamr yamr(fn, M, R);

    yamr.Split();
    MY_DEBUG_ONLY(cout << "Splitting's done" << endl;)

    string mapper_condition;

//    mr_type mapper = [mapper_condition](const string &str) -> vector<string>
//    {
//        vector<string> res;
//        res.emplace_back(str);
//        return res; // этот маппер пока просто заглушка - возвращает вектор из одной строки - исходной строки
//    };

    m_type mapper = [mapper_condition] (const string &str) mutable -> string
    {
        if (str.empty())
            return mapper_condition;

        if (mapper_condition.empty())
        {
            mapper_condition = str;
        }
        else
        {
            auto res = mismatch(mapper_condition.begin(), mapper_condition.end(), str.begin());

            //size_t p = str.
            //mapper_condition = str.substr(0, )
            mapper_condition.resize(distance(str.begin(), res.second));
            copy(str.begin(), res.second, mapper_condition.begin());
        }

        return mapper_condition;
    };

    yamr.Map(mapper);
    MY_DEBUG_ONLY(cout << "Mapping's done" << endl;)


    r_type reducer = [] (string v) mutable -> string
    {
        return v;
//        if (v.empty())
//        {
//            return string();
//        }
//        else
//        {
//            auto it = min_element(v.begin(), v.end());
//            return *it;
//        }
    };

    yamr.Reduce(reducer);
    MY_DEBUG_ONLY(cout << "Reducing's done" << endl;)

    cout << "Done." << endl;
    return 0;
}

