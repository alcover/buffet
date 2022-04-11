#ifndef LOG_H
#define LOG_H

	#define LOG(args...) do{ printf(args); printf("\n"); fflush(stdout);} while(0)
	#define ERR(args...) do{fprintf(stderr,args);} while(0);

	#define LOGS(s,msg) do{ printf("%s : '%s'\n", msg, s); 		fflush(stdout); } while(0)
	#define LOGI(i,msg) do{ printf("%s : %d\n", msg, (int)(i)); 	fflush(stdout); } while(0)
	#define LOGVS(id) 	do{ printf("%s : '%s'\n", #id, id); 	fflush(stdout); } while(0)
	#define LOGVI(id) 	do{ printf("%s : %d\n", #id, (int)id); 	fflush(stdout); } while(0)

#endif