#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <time.h>
#include "sqlite3.h"
#include "cRunWatch.h" // https://ravenspoint.wordpress.com/2010/06/16/timing/

std::vector<std::vector<int>> vdata;

class cPairStorage
{
    std::vector<std::pair<int, int>> vPair;
    sqlite3 *db;
    char *dbErrMsg;
    int transactionCount;

public:
    cPairStorage();

    void add(int r1, int r2)
    {
        vPair.push_back(std::make_pair(r1, r2));
        if (vPair.size() > transactionCount)
            writeDB();
    }

    void writeDB();

    int count();

    std::pair<int, int> get(int index);
};

cPairStorage pairStore;

cPairStorage::cPairStorage()
: transactionCount(500)
{
    int ret = sqlite3_open("pair.db", &db);
    if (ret)
        throw std::runtime_error("failed to open db");
    ret = sqlite3_exec(db,
                       "CREATE TABLE IF NOT EXISTS pair (r1, r2);",
                       0, 0, &dbErrMsg);
    ret = sqlite3_exec(db,
                       "DELETE FROM pair;",
                       0, 0, &dbErrMsg);
    ret = sqlite3_exec(db,
                       "PRAGMA schema.synchronous = 0;",
                       0, 0, &dbErrMsg);
}

void cPairStorage::writeDB()
{
    //raven::set::cRunWatch aWatcher("writeDB");

    sqlite3_stmt *stmt;
    int ret = sqlite3_prepare_v2(
        db,
        "INSERT INTO pair VALUES ( ?1, ?2 );",
        -1, &stmt, 0);

    ret = sqlite3_exec(
        db,
        "BEGIN TRANSACTION;",
        0, 0, &dbErrMsg);

    for (auto &p : vPair)
    {
        ret = sqlite3_bind_int(stmt, 1, p.first);
        ret = sqlite3_bind_int(stmt, 2, p.second);
        ret = sqlite3_step(stmt);
        ret = sqlite3_reset(stmt);
    }

    ret = sqlite3_exec(
        db,
        "END TRANSACTION;",
        0, 0, &dbErrMsg);

    //std::cout << "stored " << vPair.size() << "\n";

    vPair.clear();
}

int cPairStorage::count()
{
    int ret;

    sqlite3_stmt *stmt;
    ret = sqlite3_prepare_v2(
        db,
        "SELECT count(*) FROM pair;",
        -1, &stmt, 0);
    ret = sqlite3_step(stmt);

    int count = sqlite3_column_int(stmt, 0);
    ret = sqlite3_reset(stmt);
    return count;
}

std::pair<int, int> cPairStorage::get(int index)
{
    if (0 > index || index >= count())
        throw std::runtime_error("bad pair index");

    std::pair<int, int> pair;
    int ret;
    sqlite3_stmt *stmt;
    ret = sqlite3_prepare_v2(
        db,
        "SELECT * FROM pair WHERE rowid = ?1;",
        -1, &stmt, 0);
    ret = sqlite3_bind_int(stmt, 1, index);
    ret = sqlite3_step(stmt);
    pair.first = sqlite3_column_int(stmt, 0);
    pair.second = sqlite3_column_int(stmt, 1);
    ret = sqlite3_reset(stmt);
    return pair;
}

void generateRandom(
    int colCount,
    int rowCount,
    int maxValue)
{
    srand(time(NULL));
    for (int krow = 0; krow < rowCount; krow++)
    {
        std::vector<int> vrow;
        for (int kcol = 0; kcol < colCount; kcol++)
            vrow.push_back(rand() % maxValue + 1);
        vdata.push_back(vrow);
    }
}

bool isPair(int r1, int r2)
{
    auto &v1 = vdata[r1];
    auto &v2 = vdata[r2];
    for (int kc1 = 0; kc1 < v1.size(); kc1++)
    {
        for (int kc2 = kc1 + 1; kc2 < v1.size(); kc2++)
        {
            int tv = v1[kc1];
            if (tv != v1[kc2])
                continue;
            if (tv != v2[kc1])
                continue;
            if (tv != v2[kc2])
                continue;
            return true;
        }
    }
    return false;
}
void findPairs()
{

    raven::set::cRunWatch aWatcher("findPairs");

    int colCount = vdata[0].size();

    for (int kr1 = 0; kr1 < vdata.size(); kr1++)
    {
        bool pairPossible = false;
        for (int kc1 = 0; kc1 < colCount; kc1++)
            for (int kc2 = kc1 + 1; kc2 < colCount; kc2++)
                if (vdata[kr1][kc1] == vdata[kr1][kc2])
                {
                    // row has two cols with equal values
                    // so it can be part of a row pair
                    pairPossible = true;
                    break;
                }
        if (!pairPossible)
            continue;
        for (int kr2 = kr1 + 1; kr2 < vdata.size(); kr2++)
            if (isPair(kr1, kr2))
                pairStore.add(kr1, kr2);
    }

    pairStore.writeDB();
}

void display()
{
    std::cout << "\nFound " << pairStore.count() << " pairs in " << vdata.size() << " records\n\n";
    std::cout << "First 2 pairs found:\n\n";
    for (int kp = 0; kp < 2; kp++)
    {
        auto p = pairStore.get(kp+1);
        for (int v : vdata[p.first])
            std::cout << v << " ";
        std::cout << "\n";
        for (int v : vdata[p.second])
            std::cout << v << " ";
        std::cout << "\n\n";
    }

    raven::set::cRunWatch::Report();
}

main(int ac, char *argc[])
{
    int rowCount = 10;
    if (ac == 2)
        rowCount = atoi(argc[1]);

    raven::set::cRunWatch::Start();

    generateRandom(
        5,        // columns
        rowCount, // rows
        20);      // max value

    findPairs();

    display();

    return 0;
}
