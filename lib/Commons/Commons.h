#define MAIN_DEBUG

#define DEBUG_PORT Serial

#ifdef MAIN_DEBUG
#define DB_PRINT(X) DEBUG_PORT.print(X);
#else
#define DB_PRINT(X) // nothing
#endif

#ifdef MAIN_DEBUG
#define DB_PRINTLN(X) DEBUG_PORT.println(X);
#else
#define DB_PRINTLN(X) // nothing
#endif