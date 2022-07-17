#ifndef ALCO_LOG_H
#define ALCO_LOG_H

// todo generics ?

#define ERR(args...) do { \
    fprintf(stderr,"line %d: ",__LINE__);  \
    fprintf(stderr,args); \
} while(0)

#ifdef NDEBUG

    #define WARN(...)   ((void)0)
    #define LOG(...)    ((void)0)
    #define LOGS(s,msg) ((void)0)
    #define LOGI(id)    ((void)0)
    #define LOGVS(id)   ((void)0)
    #define LOGVI(id)   ((void)0)

#else

#define WARN(args...) do { \
    fprintf(stderr,"WARN %s:%d ", __func__, __LINE__);  \
    fprintf(stderr,args); \
} while(0)

#define LOG(args...) do { \
    printf(args);  \
    printf("\n"); \
    fflush(stdout); \
} while(0)

#define LOGS(s,msg) do { \
    printf("%s : '%s'\n", msg, s); \
    fflush(stdout); \
} while(0)

#define LOGI(i,msg) do { \
    printf("%s : %d\n", msg, (int)(i)); \
    fflush(stdout); \
} while(0)

#define LOGVS(id)   do { \
    printf("%s : '%s'\n", #id, id); \
    fflush(stdout); \
} while(0)

#define LOGVI(id)   do { \
    printf("%s : %d\n", #id, (int)id); \
    fflush(stdout); \
} while(0)

#endif
#endif