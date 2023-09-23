#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <time.h>
#include <thread>
#include <mutex>
#include "sqlite3.h"
#include "cRunWatch.h" // https://ravenspoint.wordpress.com/2010/06/16/timing/
#include "cCommandParser.h"

class cPairStorage
{
    std::vector<std::pair<int, int>> vPair;
    sqlite3 *db;
    char *dbErrMsg;
    int transactionCount;

public:
    cPairStorage();

    void setTransactionCount(int count);

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
    std::mutex mtx;

    // configuration params

    bool fMultiThread; /// true for multhreading
    int rowCount;      /// number random data rows to generate
    int randSeed;      /// random generator seed

    bool isTwoValues(int r);

    bool isPair(int r1, int r2);

    /// @brief run first part data search in own thread
    /// @param first first row
    /// @param last  last row
    /// @return thread handle
    std::thread findPairsRange1(
        int first,
        int last);

    /// @brief run second part data search in own thread
    /// @param first first row
    /// @param last  last row
    /// @return thread handle
    std::thread findPairsRange2(
        int first,
        int last);

    /// @brief search part of data
    /// @param first    first row
    /// @param last last roe
    void findPairsRange(
        int first,
        int last);

public:
    cMatcher();

    void parseCommandLine(
        int argc, char *argv[]);

    void generateRandom(
        int colCount,
        int maxValue);

    void set(const std::vector<std::vector<int>> &data);

    void readfile(const std::string &fname);

    // Search data for paired rows
    void findPairs();

    int pairCount();

    void display();

    static bool test();
};