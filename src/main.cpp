#include "RecordMatcher.h"

main(int ac, char *argc[])
{
    int rowCount = 10;
    if (ac == 2)
        rowCount = atoi(argc[1]);

    raven::set::cRunWatch::Start();

    cMatcher matcher;

    if( ! matcher.test() )
        return 1;

    matcher.generateRandom(
        5,        // columns
        rowCount, // rows
        20);      // max value

    matcher.findPairs();

    matcher.display();

    return 0;
}
