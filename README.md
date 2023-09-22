# Problem

Find all record pairs that share the same value in two, or more, of the same columns.  [Details](https://github.com/JamesBremner/RecordMatcher/wiki/Matching-Record-Pairs).

# Usage

`>matcher`

With no command line paramters, the app will generate a random data set with 10 records

`>matcher 100000`

With a command line parameter, the app will generate a random data set with the specified number of records

# Performance

Finds ~3,000,000 pairs in 100,000 records in 110 seconds on a modest laptop.
