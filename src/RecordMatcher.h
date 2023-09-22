#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <time.h>
#include "sqlite3.h"
#include "cRunWatch.h" // https://ravenspoint.wordpress.com/2010/06/16/timing/

class cPairStorage
{
    std::vector<std::pair<int, int>> vPair;
    sqlite3 *db;
    char *dbErrMsg;
    int transactionCount;

public:
    cPairStorage();

    void clear();

    void add(int r1, int r2);

    void writeDB();

    int count();

    std::pair<int, int> get(int index);
};

class cMatcher
{
    std::vector<std::vector<int>> vdata;
    cPairStorage pairStore;

    bool isPair(int r1, int r2);

public:
    void generateRandom(
        int colCount,
        int rowCount,
        int maxValue);

    void set( const std::vector<std::vector<int>>& data );

    void findPairs();

    int pairCount();

    void display();

    static bool test();
};