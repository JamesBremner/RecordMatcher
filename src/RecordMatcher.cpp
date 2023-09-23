#include "RecordMatcher.h"

cPairStorage::cPairStorage()
    : transactionCount(1000)
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

void cPairStorage::setTransactionCount( int count )
{
    transactionCount = count;
}

void cPairStorage::clear()
{
    vPair.clear();
    sqlite3_exec(db,
                 "DELETE FROM pair;",
                 0, 0, &dbErrMsg);
}

void cPairStorage::add(int r1, int r2)
{
    vPair.push_back(std::make_pair(r1, r2));
    if (vPair.size() > transactionCount)
        writeDB();
}

void cPairStorage::writeDB()
{
    // raven::set::cRunWatch aWatcher("writeDB");

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

    // std::cout << "stored " << vPair.size() << "\n";

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

cMatcher::cMatcher()
: fMultiThread( false )
{}

void cMatcher::generateRandom(
    int colCount,
    int maxValue)
{
    if( randSeed )
        srand( randSeed);
    else
        srand(time(NULL));

    for (int krow = 0; krow < rowCount; krow++)
    {
        std::vector<int> vrow;
        for (int kcol = 0; kcol < colCount; kcol++)
            vrow.push_back(rand() % maxValue + 1);
        vdata.push_back(vrow);
    }
}

 void cMatcher::parseCommandLine(int argc, char* argv[])
 {
    raven::set::cCommandParser P;
    P.add("help", "\tproduce help message");
    P.add("rows", "count\tnumber of records( default: 10 )");
    P.add("trans", "count\tnumber of pairs to write in one DB transaction ( default: 1000 )");
    P.add("multi", "\tmultithreading ( default: off )","bool");
    P.add("seed", "value\trandom seed ( default: set from system clock)");
    
    P.parse(argc, argv);

    rowCount = atoi(P.value("rows").c_str());
    if( rowCount == 0 )
        rowCount = 10;

    fMultiThread = ( P.value("multi") == "t");

    int trans  = atoi(P.value("trans").c_str());
    if( trans > 0 )
        pairStore.setTransactionCount( trans );

    randSeed = atoi(P.value("seed").c_str());

 }

void cMatcher::set(const std::vector<std::vector<int>> &data)
{
    vdata = data;
}

void cMatcher::readfile(const std::string &fname)
{
    pairStore.clear();
    vdata.clear();
    std::ifstream ifs(fname);
    if (!ifs.is_open())
        throw std::runtime_error(
            "Cannot open " + fname);
    std::string line;
    while (getline(ifs, line))
    {
        std::istringstream ss(line);
        int value;
        std::vector<int> row;
        while (ss.good())
        {
            ss >> value;
            row.push_back(value);
        }
        vdata.push_back(row);
    }
}

bool cMatcher::isPair(int r1, int r2)
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

bool cMatcher::isTwoValues(int r)
{
    int colCount = vdata[0].size();

    for (int kc1 = 0; kc1 < colCount; kc1++)
    {
        for (int kc2 = kc1 + 1; kc2 < colCount; kc2++)
        {
            if (vdata[r][kc1] == vdata[r][kc2])
            {
                // row has two cols with equal values
                // so it can be part of a row pair
                return true;
            }
        }
    }
    return false;
}
void cMatcher::findPairs()
{
    raven::set::cRunWatch aWatcher("findPairs");

    pairStore.clear();

    if (!fMultiThread)
    {
        // search entire data in this process
        findPairsRange(0, vdata.size() - 1);
    }
    else
    {
        // split data int two parts
        // balanced so that both parts can be searched in about the same time
        int splitRecordCount = vdata.size()/4;

        // search each part in its own process
        std::thread th1 = findPairsRange1(
            0, splitRecordCount - 1);
        std::thread th2 = findPairsRange2(
            splitRecordCount, vdata.size() - 1);

        // wait for both searches to complete
        th1.join();
        th2.join();
    }

    pairStore.writeDB();
}

void cMatcher::findPairsRange(
    int first,
    int last)
{
    for (int kr1 = first; kr1 <= last; kr1++)
    {
        if (!isTwoValues(kr1))
            continue;

        // row kr1 contains at leats two cols with equal values
        // so it may be the first record in some matched pairs

        // loop over later rows checking for matches
        for (int kr2 = kr1 + 1; kr2 < vdata.size(); kr2++)
            if (isPair(kr1, kr2))
            {
                std::lock_guard<std::mutex> lck(mtx);
                pairStore.add(kr1, kr2);
            }
    }
}
std::thread cMatcher::findPairsRange1(
    int first,
    int last)
{
    raven::set::cRunWatch aWatcher("findPairsRange1");
    return std::thread(&cMatcher::findPairsRange, this, first, last);
}
std::thread cMatcher::findPairsRange2(
    int first,
    int last)
{
    raven::set::cRunWatch aWatcher("findPairsRange2");
    return std::thread(&cMatcher::findPairsRange, this, first, last);
}

void cMatcher::display()
{
    std::cout << "\nFound " << pairStore.count() << " pairs in " << vdata.size() << " records\n\n";
    std::cout << "First 2 pairs found:\n\n";
    for (int kp = 0; kp < 2; kp++)
    {
        auto p = pairStore.get(kp + 1);
        for (int v : vdata[p.first])
            std::cout << v << " ";
        std::cout << "\n";
        for (int v : vdata[p.second])
            std::cout << v << " ";
        std::cout << "\n\n";
    }

    raven::set::cRunWatch::Report();
}

int cMatcher::pairCount()
{
    return pairStore.count();
}

bool cMatcher::test()
{
    bool res = true;
    cMatcher M;
    M.set({{1, 1, 1, 2},
           {2, 1, 1, 7},
           {1, 1, 1, 2},
           {2, 3, 1, 7}});
    if (!M.isPair(0, 1))
        res = false;
    if (!M.isPair(1, 2))
        res = false;
    M.findPairs();
    if (M.pairCount() != 3)
        res = false;
    if (!res)
        std::cout << "unit test failed\n";
    else
        std::cout << "unit tests passed\n";

    return res;
}
