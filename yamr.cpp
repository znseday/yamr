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

void Yamr::Map(/*const*/ mr_type &mapper) // нужен ли const?
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
            vector<string> mapping_res_all;

            //ofstream f("mapping_res_" + to_string(i) + ".txt");

            for (const auto &s : SectionsDataTemp[i])
            {

//                f << mapper(s) << endl;

                mapping_res = mapper(s);

                for (const auto &v : mapping_res)
                    mapping_res_all.emplace_back(v);

//                vector<string> res = mapper(s);

//                sort(res.begin(), res.end()); // is it ok to do like that in a thread /w mutex ???

//                MY_DEBUG_ONLY(
//                    for(const auto &s : res) {  m_console.lock(); cout << "Mapper [" << i << "] res: " << s << endl; m_console.unlock();}
//                             )
            }

//            sort(mapping_res_all.begin(), mapping_res_all.end(), greater<>());
            sort(mapping_res_all.begin(), mapping_res_all.end());

            ofstream f("mapping_res_" + to_string(i) + ".txt");
            for (const auto &s : mapping_res_all)
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

void Yamr::Reduce(/*const*/ mr_type &reducer) // нужен ли const?
{
    vector<thread*> ts;
    ts.reserve(R);

    mutex m;

    vector<vector<string>> map_data(M);

    vector<ifstream> in_fs(M);
    for (size_t i = 0; i < M; i++)
    {
        in_fs[i].open("mapping_res_" + to_string(i) + ".txt");

        string line;
        while ( getline(in_fs[i], line) )
            map_data[i].emplace_back(line);
    }

    vector<string> all_data;
    vector<string> tv = map_data[0];
    for (size_t i = 1; i < M; i++)
    {
        all_data.clear();
        merge(tv.begin(), tv.end(), map_data[i].begin(), map_data[i].end(), std::back_inserter(all_data));
        tv = all_data;
        //merge(all_data.begin(), all_data.end(), map_data[i].begin(), map_data[i].end(), all_data.end());
    }

    MY_DEBUG_ONLY(
        ofstream f_test("all_data.txt");
        for (const auto &s : all_data)
            f_test << s << endl;
        f_test.close();
    )

    size_t count = all_data.size();
    vector<size_t> indexes(R+1);
    size_t block_size = count/R;
    indexes[0] = 0;
    for (size_t i = 1; i < R; ++i)
    {
        indexes[i] = i*block_size + 1;
    }
    indexes[R] = count;

    MY_DEBUG_ONLY(
        for (auto ind : indexes)
            cout << "ind = " << ind << endl;
    )

    for (size_t i = 0; i < R; i++)
    {
        ts.emplace_back( new thread( [reducer, i, &m, &in_fs, &indexes, &all_data, this]()
        {

            //vector<string> candidates; // ведь у каждого потока будет своя копия вектора?
            //candidates.reserve(M);

            ofstream f("reducing_res_" + to_string(i) + ".txt");

            for (size_t j = indexes[i]; j < indexes[i+1]; ++j)
            {
                auto res = reducer(all_data[j]);
                for (const auto &s : res)
                    f << s << endl;
            }

//            m.lock();
//            for (size_t j = 0; j < M; j++)
//            {
//                string line;
//                if ( getline(in_fs[j], line) )
//                    candidates.emplace_back(move(line));
//            }
//            m.unlock();

//            string tt;
//            if (candidates.empty())
//            {
//                tt = string();
//            }
//            else
//            {
//                auto it = min_element(candidates.begin(), candidates.end());
//                tt = *it;
//            }

//            auto res = reducer("qwerty");
//            for (const auto &s : res)
//                f << s << endl;

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


