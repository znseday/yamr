#include <iostream>
#include <cassert>
#include <fstream>
#include <chrono>
#include <thread>


#include "yamr.h"

using namespace std;

//std::mutex console_m;

bool IsDebugOutput = false;

Yamr::Yamr(const std::string _fn, int _M, int _R) : fn(_fn), M(_M), R(_R)  //, SectionsData(_M)
{
    SectionsInfo.reserve(M);
}


void Yamr::Split()
{
    ifstream f(fn, ios::binary);
    if (!f)
    {
        cout << "My error: input file not found" << endl;
        exit(0);
    }
    f.seekg(0, ios::end);
    size_t fs = f.tellg();
    MY_DEBUG_ONLY(cout << "file_size = " << fs << endl;)
    f.seekg(0, ios::beg);

    size_t averBlockSize = fs/M;
    MY_DEBUG_ONLY(cout << "averBlockSize = " << averBlockSize << endl;)

    size_t curPos = 0;
    SectionsInfo.push_back(0);

    for (size_t i = 0; i < M-1; i++)
    {
        curPos = (i+1)*averBlockSize;

        f.seekg(curPos+1, ios::beg);

        char c1;// c2;

        do
        {
            f.get(c1);
            curPos++;
            //f.get(c2);
        }
        while (curPos <= fs && !(c1 == '\r' || c1 == '\n'));

        if (c1 == '\n')
            curPos += 1;
        else
            curPos += 2;

        SectionsInfo.push_back(curPos);
    }

    MY_DEBUG_ONLY(
        for (size_t i = 0; i < SectionsInfo.size(); i++)
        {
            f.seekg(SectionsInfo[i], ios::beg);
            cout << "p" << i << " = " << SectionsInfo[i];
            char c;
            f.get(c);
            cout << "\t  first char = " << c << endl;
        }
    )

}
//--------------------------------------------------------------

void Yamr::Map(/*const*/ m_type &mapper) // нужен ли const?
{
    vector<thread*> ts;
    ts.reserve(M);

    mutex m;

    ifstream f(fn);
    f.seekg(0, ios::end);
    size_t fs = f.tellg();
    MY_DEBUG_ONLY(  m_console.lock();cout << "file_size = " << fs << endl; m_console.unlock();)
    f.seekg(0, ios::beg);

    vector<vector<string>> SectionsDataTemp(M);
    MY_DEBUG_ONLY( m_console.lock(); cout << "SectionsDataTemp.size() = " << SectionsDataTemp.size() << endl; m_console.unlock(); )

    for (size_t i = 0; i < M; i++)
    {
        // делаем у каждого потока свою копию маппера, чnобы у каждого было свое независимое состояние
        ts.emplace_back( new thread( [&m, i, &f, &SectionsDataTemp, mapper, this](size_t start, size_t end)
        {
            m.lock();
            MY_DEBUG_ONLY( m_console.lock();  cout << "thread " << i << ": start: " << start << "   end: " << end << endl; m_console.unlock(); )

            f.seekg(start, ios::beg);
            string line;
            size_t curPos = start;
            do
            {
                getline(f, line);
                MY_DEBUG_ONLY( m_console.lock();  cout << "thread " << i << ": line: " << line << endl; m_console.unlock(); )

                //curPos = f.tellg();  // doesn't work or works wierd (maybe it reads more than expected). Maybe it relates with buffering

                curPos += line.size() + 2;  // It works only if it's 2. Why? Is it cr+lf?

                MY_DEBUG_ONLY( m_console.lock();  cout << "thread " << i << ": curPos: " << curPos << endl; m_console.unlock(); )

                SectionsDataTemp[i].emplace_back(line);

            } while (curPos < end);

            m.unlock();

            vector<string> mapping_res;

            //ofstream f("mapping_res_" + to_string(i) + ".txt");

            for (const auto &s : SectionsDataTemp[i])
            {

//                f << mapper(s) << endl;

                mapping_res.emplace_back(mapper(s));

//                vector<string> res = mapper(s);

//                sort(res.begin(), res.end()); // is it ok to do like that in a thread /w mutex ???

//                MY_DEBUG_ONLY(
//                    for(const auto &s : res) {  m_console.lock(); cout << "Mapper [" << i << "] res: " << s << endl; m_console.unlock();}
//                             )
            }

            sort(mapping_res.begin(), mapping_res.end());

            ofstream f("mapping_res_" + to_string(i) + ".txt");
            for (const auto &s : mapping_res)
                f << s << endl;

            f.close();

        }, // end of lambda
        SectionsInfo[i], (i<M-1)?(SectionsInfo[i+1]):(fs-1)  // lambda params
                             )   );
    } // end for

    for (auto t : ts)
        t->join();

    for (auto t : ts)
        delete t;

//    SectionsData = move(SectionsDataTemp);

//    MY_DEBUG_ONLY( cout << "SectionsData.size() = " << SectionsData.size() << endl; )

//    MY_DEBUG_ONLY(
//    for (size_t i = 0; i < M; i++)
//        for (const auto &s : SectionsData[i])
//            cout << "Section " << i << ": " << s << endl;
//    )

}
//--------------------------------------------------------------

void Yamr::Reduce(/*const*/ r_type &reducer) // нужен ли const?
{
    vector<thread*> ts;
    ts.reserve(R);

    mutex m;

    vector<ifstream> in_fs(M);
    for (size_t i = 0; i < M; i++)
    {
        in_fs[i].open("mapping_res_" + to_string(i) + ".txt");
    }

    for (size_t i = 0; i < R; i++)
    {
        ts.emplace_back( new thread( [reducer, i, &m, &in_fs, this]()
        {
            vector<string> candidates; // ведь у каждого потока будет своя копия вектора?
            candidates.reserve(M);

            ofstream f("reducing_res_" + to_string(i) + ".txt");

            m.lock();

            for (size_t j = 0; j < M; j++)
            {
                string line;
                if ( getline(in_fs[j], line) )
                    candidates.emplace_back(move(line));
            }

            m.unlock();

            string tt;
            if (candidates.empty())
            {
                tt = string();
            }
            else
            {
                auto it = min_element(candidates.begin(), candidates.end());
                tt = *it;
            }

            f << reducer(tt) << endl;

//            for (auto data : DataForReducers)
//            {
//                // to do
//                auto res = reducer(str);
//                // to do
//            }

            f.close();

        }  // end lambda
        )  // end thread
        ); // end emplace_back
    } // end for

    for (auto t : ts)
        t->join();

    for (size_t i = 0; i < M; i++)
    {
        in_fs[i].close();
    }

    for (auto t : ts)
        delete t;
}
//--------------------------------------------------------------


