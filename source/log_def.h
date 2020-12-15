#ifndef LOG_DEF_H
#define LOG_DEF_H 1
#include <stdio.h>

#ifdef MYDBG
#include <chrono>
#endif

/*          Colors           */
#ifndef _MSC_VER
#define RED "\e[1;31m"
#define BLU "\e[1;34m"
#define YEL "\e[1;33m"
#define PUR "\e[1;35m"
#else
#define RED ""
#define BLU ""
#define YEL ""
#define PUR ""
#endif

/*          Log Printer      */
#define LOGI(s, ...) fprintf(stderr,BLU "[INFO] " s "\n", ##__VA_ARGS__) 
#define LOGW(s, ...) fprintf(stderr,YEL "[WARNING] " s "\n", ##__VA_ARGS__) 
#define LOGE(s, ...) fprintf(stderr,RED "[ERROR] " s "\n", ##__VA_ARGS__) 

#ifdef MYDBG
#define LOGD(s, ...) fprintf(stderr,PUR "[DEBUG] " s "\n", ##__VA_ARGS__) 

/*          Time test        */
#define CNTIME auto __START = std::chrono::steady_clock::now()
#define OPTIME(s) LOGD(s "time used %lf", std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - __START).count())


#else
#define LOGD(s, ...) do{}while(0)
#define CNTIME do{}while(0)
#define OPTIME(s) do{}while(0)
#endif




#endif