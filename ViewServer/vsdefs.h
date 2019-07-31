// Linux version of stdafx.h for ViewServerLinux only
#ifndef _VSDEFS_H
#define _VSDEFS_H

#define FEEDTASK_STRINGKEY
#define FEEDTASK_MULTITASK
#define MAKE_STATIC
#define WSTRING_STRTOKE

// Various stages of code improvement
// I removed all the #ifdef-ed-out. Code now corresponds to these parameters.
// An attempt to simply what's stored to minimum
//#undef STORE_MINIMUM
// Define to write details to the DB file; overrides STORE_MINIMUM
//#undef DETAILS_INLINE
// Define to write details to separate files; overrides DETAILS_SEPFILE
//#define DETAILS_SEPFILE
// Define to write details synchronously; requires DETAILS_SEPFILE
//#define DETAILS_SYNCWRITE
// Define to distribute over multiple processes
//#define MULTI_PROCESS
// Define to do overlapped detail reads for speed up
//#define OVL_DETAIL_READ
// Define to use MRU cache for each detail thread
// In testing, MRU achieves 0,11,18,43% hit ratios
//#define USE_MRU_CACHE
// Define to replace VSParentOrder with DiskIndex or MemIndex
//#define PARENT_CHILD_MAP
// Define to test unloading the map for terminated orders to see if we can reach 50M fix messages
#undef NOMAP_TERM_ORDERS
// Define to unload terminated orders immediately
#define UNLOAD_TERM_ORDERS
// Define to not keep parent/child associations. Can't run full data set 48M orders in 2GB memory.
//#undef NOCHILDMAP
// Define to use Jason's DiskIndex.lib instead of memindex.cpp to achieve > 14M orders due to 2GB mem limit on 32-bit OS
#undef USE_DISK_INDEX
// Define TBD
//#undef USE_TREE_INDEX
// Define to include symbol, account, and ecnorderid indices
//#define MORE_INDICES
// Define to track order and cummulative fill quantity
//#define TRACK_QTY
// Define to write all fills to a separate file
//#define FILLS_FILE
// Define for MemIndex 1st-level hash before multimap to reduce items per map
#undef MULTI_HASH
// Define to use a different database for each app instance (MULTI_HASH not needed when set)
//#define SEP_APPSYS_MAPS
// Define to load the database in a separate thread
//#define DB_ASYNC_LOAD
// Define to not require AppInstID== where condition on queries
//#define NO_QUERY_APPINST_REQD
// Define to hold child orders until the parent order arrives
#define HOLD_IQ_CHILD_ORDERS
// Define to retransmit journal pages
#define RETRANS_JOURNAL
// End of version 1 defines Nov 11, 2010

// Version 2 features
// Define for AppInstance->AppSystem pointers
//#define HAVE_APPSYSPTR
// Define to enforce user entitlements
//#define ENFORCE_ENTITLEMENTS
// Define to aggregate order totals to AppInstance and AppSystem
//#define HAVE_ACCOUNTS
// Define to support queries on multiple values for a key (requires HAVE_APPSYSPTR)
//#define MULTI_VALUE_QUERIES
// Define to support server-to-server query proxy for remote regions and alternate dates
//#define REGION_QUERY
// Define for user query result thread pool to protect and speed up FIX processing thread
// For now this is turned off because it precludes the MRU cache and order cache, which slows down processing too much.
// Needs to be tested with fusion I/O, for which those caches are not used anyway.
#undef TASK_THREAD_POOL
// Define to support IQ DNS tables
#define IQDNS
// Define to support IQ order staging tables
#undef IQOS
// Define to use one detail file only for $CLBACKUP$
#define SINGLE_CLBACKUP_FILE

// Debugging defines (should never be defined for production release)
// Define to test throughput without DB writes; overrides all
#undef NO_DB_WRITES
// Define to test throughput without FIX detail logging
#undef NO_FIX_WRITES
// Define to test hash distribution on order databaes load (requires !USE_DISK_INDEX)
#undef TEST_HASH_DIST
// Define TBD
#undef DEBUG_ORDER_KEYS
// Define TBD
#undef TEST_ALL_ORDERIDS
// Define to vary FIX playback tags 11,41,37,and 5055 to increase load for 200M msg benchmark
#undef MULTIPLY_FIX //4
// Define to test 4 days worth in one server
#undef MULTI_DAY_REPLAY
// Define until overvlapped read thread fixed (done)
//#undef NO_OVL_READ_DETAILS
// Define to write journal_<alias>.txt and journal_<usr>.txt debug files
#define DEBUG_JOURNAL
// Define to validate order write locations
#undef VALIDATE_DBWRITES

//Define for Spectrum specific functionality
#undef SPECTRUM  //DT9386
// Define to support IQ drops from sysmon proxy
#define IQSMP
// Define to support multiple days in same server
#define MULTI_DAY
// Define to support active FIX connections as server
#define FIX_SERVER
// Define to support auxkey expressions
#define AUXKEY_EXPR
// Define to support multiple days history in same server
#define MULTI_DAY_HIST
// Define to store positions and accounts as different record types
#define MULTI_RECTYPES
// Define to support Redi OleDB connection
#define REDIOLEDBCON
// Define to connect to IQ SQL Server database
#undef IQDEVTEST
// Define to receive away drops from legacy FIX servers
#define AWAY_FIXSERVER
// Define to import oats files to SQL Server database
#undef REDISQLOATS
// Define to discontinue efills file generation
#define NO_EFILLS_FILE
// Define to multiplex ITEMLOC across dates
#define MULTI_DAY_ITEMLOC
// Define for auto-appinstance creation from DNS
#undef DNS_INSTANCES
// Define while RTECHOATS drop comes from GS
#undef GS_RTECHOATS

#endif//_VSDEFS_H
