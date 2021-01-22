/** used as in return value of state() call */
#define MOD_IS_READY          1
#define MOD_WRONG_API         2
#define MOD_STARTED           4 /* source, backend */
#define MOD_NEED_FILTER       8 /* only source */
/* reserved */
#define MOD_TYPE_SOURCE    1024 
#define MOD_TYPE_FILTER    2048
#define MOD_TYPE_BACKEND   4096

#define MATCH_DEFSCORE 10
