#!/bin/sh
date +"#define __YEAR__ %Y" > ../include/build_time.h

date +"#define __MONTH__ %m" >> ../include/build_time.h

date +"#define __DAY__ %d" >> ../include/build_time.h

date +"#define __HOURS__ %H" >> ../include/build_time.h

date +"#define __MINUTES__ %M" >> ../include/build_time.h

date +"#define __SECONDS__ %S" >> ../include/build_time.h

date +"#define ____TIMESTAMP____ %s" >> ../include/build_time.h

date +"#define __DAY_OF_WEEK__ %w" >> ../include/build_time.h
