#include <iostream>
#include <cassert>
#include <fstream>
#include <chrono>
#include <thread>


#include "yamr.h"

using namespace std;

//std::mutex console_m;

bool IsDebugOutput = false;

//size_t Reducer::Number = 0;

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
        ts.emplace_back( new thread( [&m, i, &f, &SectionsDataTemp, &mapper, this](size_t start, size_t end)
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

            for (size_t i = 0; i < M; i++)
                for (const auto &s : SectionsDataTemp[i])
                {
                    vector<string> res = mapper(s);

                    sort(res.begin(), res.end()); // is it ok to do like that in a thread /w mutex ???

                    MY_DEBUG_ONLY(
                        for(const auto &s : res) {  m_console.lock(); cout << "Mapper [" << i << "] res: " << s << endl; m_console.unlock();}
                                 )
                }

            //sort(SectionsDataTemp[i].begin(), SectionsDataTemp[i].end()); // is it ok to do like that in a thread ???

        }, // end of lambda
        SectionsInfo[i], (i<M-1)?(SectionsInfo[i+1]):(fs-1)  // lambda params
                             )   );
    } // end for

    for (auto t : ts)
        t->join();

    for (auto t : ts)
        delete t;

    SectionsData = move(SectionsDataTemp);

    MY_DEBUG_ONLY( cout << "SectionsData.size() = " << SectionsData.size() << endl; )

    MY_DEBUG_ONLY(
    for (size_t i = 0; i < M; i++)
        for (const auto &s : SectionsData[i])
            cout << "Section " << i << ": " << s << endl;
    )

}
//--------------------------------------------------------------

//void Yamr::Shuffle()
//{
//    DataForReducers.resize(R);

//    vector<size_t> pointers(M, 0);
//    vector<string> curs(M);

//    vector<string> results;

//    //size_t nLines = 0;
//    //for (const auto & sd : SectionsData)
//    //    nLines += sd.size();

//    bool isDone = false;
//    do
//    {
//        for (size_t i = 0; i < M; i++)
//            curs[i] = SectionsData[i][pointers[i]];

//        size_t iWorkBlock = min_element(curs.begin(), curs.end()) - curs.begin();

//        results.push_back(SectionsData[iWorkBlock][pointers[iWorkBlock]]);

//        for (pointers[iWorkBlock]++; pointers[iWorkBlock] < SectionsData[iWorkBlock].size(); pointers[iWorkBlock]++)
//        {
//            if (SectionsData[iWorkBlock][pointers[iWorkBlock]] > curs[iWorkBlock])
//            {
//                for (size_t jBlock = 0; jBlock < M; jBlock++)
//                {
//                    if (jBlock == iWorkBlock)
//                        continue;

//                    for (size_t h = pointers[jBlock]; h < SectionsData[jBlock].size(); h++)
//                        if (SectionsData[jBlock][h] > curs[iWorkBlock])
//                        {
//                            pointers[jBlock] = h;
//                            break;
//                        }
//                }

//                break;
//            }
//        }

//        isDone = true;
//        for (size_t i = 0; i < M; i++)
//        {
//            if (pointers[i] < SectionsData[i].size()-1 )
//            {
//                isDone = false;
//                break;
//            }
//        }

//    } while (!isDone);

//    MY_DEBUG_ONLY(
//        cout << endl;
//        for (const auto &s : results)
//            cout << s << endl;
//        cout << endl;
//    )

//    size_t R_BlockSize = results.size() / R + 1;
//    for (size_t i = 0; i < results.size(); i++)
//    {
//        size_t j = i / R_BlockSize;
//        DataForReducers[j].emplace_back(std::move(results[i]));
//    }

//    MY_DEBUG_ONLY(
//        cout << endl;
//        for (size_t i = 0; i < DataForReducers.size(); i++)
//        {
//            cout << "Data for reducer " << i << ":" << endl;
//            for (const auto &s : DataForReducers[i])
//                cout << s << endl;
//        }
//        cout << endl;
//    )
//}
//--------------------------------------------------------------

//void Yamr::Reduce(/*const*/ mr_type &reducer) // нужен ли const?
//{
//    vector<thread*> ts;
//    ts.reserve(R);

//    //mutex m;

//    for (size_t i = 0; i < R; i++)
//    {
//        ts.emplace_back( new thread( [&reducer, this](const string &str)
//        {
//            ofstream f(to_string(FileNumber) + ".txt");

//            for (auto data : DataForReducers)
//            {
//                // to do
//                vector<string> res = reducer(str);
//                // to do
//            }

//            f.close();

//        }  // end lambda
//        )  // end thread
//        ); // end emplace_back
//    } // end for

//    for (auto t : ts)
//        t->join();

//    for (auto t : ts)
//        delete t;
//}
//--------------------------------------------------------------


