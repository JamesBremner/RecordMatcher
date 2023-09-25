# Problem

Find all record pairs that share the same value in two, or more, of the same columns.  [Details](https://github.com/JamesBremner/RecordMatcher/wiki/Matching-Record-Pairs).

# Usage

Command line parameters give control of configuration, to fine tune performance

```
>matcher

 --help                 produce help message
 --multi                multithreading ( default: off )
 --rows         count   number of records( default: 10 )
 --seed         value   random seed ( default: set from system clock)
 --trans        count   number of pairs to write in one DB transaction ( default: 1000 )
```

# Algorithm

```
SET T number of pairs to writ in one DB transaction
LOOP N over all records
   IF N has 2 or more identical values
      LOOP M over records N+1 to last
          LOOP C over columns
              LOOP D over cols C+1 to last
                 IF N[C] == N[D] == M[C] == M[D]
                     SAVE M,N to memory pair store
                     IF memory pair store size >= T
                         WRITE memory pair store to DB
                         CLEAR memory pair store
WRITE memory pair store to DB
```

# Performance

Finds ~6,200,000 pairs in 100,000 records in 45 seconds on a modest laptop. [Details](https://github.com/JamesBremner/RecordMatcher/wiki/Performance).
