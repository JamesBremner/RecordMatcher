#include "RecordMatcher.h"

main(int ac, char *argc[])
{
    cMatcher matcher;

    matcher.parseCommandLine( ac,argc);

    if( ! matcher.test() )
        return 1;

    raven::set::cRunWatch::Start();

    matcher.generateRandom(
        5,        // columns
        20);      // max value

    matcher.findPairs();

    matcher.display();

    return 0;
}
