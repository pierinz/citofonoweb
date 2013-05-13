#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <sqlite3.h>

//Debian and Gentoo (and maybe other distros) use different path for the same library
#ifdef json
    #include <json/json.h>
#else
    #include <json-c/json.h>
#endif

#include "libdoor.h"

#ifndef CONFPATH
    #define CONFPATH "conf/badge_daemon.conf"
#endif

#ifndef D_SIZE
    #define D_SIZE 40
#endif

char *dbfile;
int verbose, doorpin, doortime, alarmpin, alarmtime;
int loop=1;

void loadConf(){
    FILE* fp;
    char line[255],def[55],val[200];

    fp=fopen(CONFPATH, "r");
    if (!fp){
        fprintf(stderr,"File %s:\n",CONFPATH);
        perror("Error opening configuration: ");
        exit(1);
    }
    while(fgets(line,255,fp)){
        sscanf(line,"%s%s",def,val);
        if (strcmp(def,"dbfile")==0){
            /* must be large enough to contain "val" */
            dbfile=calloc(1,sizeof(val));
            strcpy(dbfile,val);
        }
        if (strcmp(def,"verbose")==0){
            verbose=atoi(val);
        }
        if (strcmp(def,"doorpin")==0){
            doorpin=atoi(val);
        }
        if (strcmp(def,"doortime")==0){
            doortime=atoi(val);
        }
        if (strcmp(def,"alarmpin")==0){
            alarmpin=atoi(val);
        }
        if (strcmp(def,"alarmtime")==0){
            alarmtime=atoi(val);
        }
    }
    fclose(fp);

    if (verbose > 1){
        fprintf(stderr,"Configuration loaded.\n");
    }
}

void jsonparse(const char* sched, int day, int* start, int* end){
    json_object *jarray, *jobj;
    jobj = json_tokener_parse(sched);

    json_object_object_foreach(jobj, key, value) { /*Passing through every array element*/
        if (atoi(key)==day){
            if (json_object_get_type(value)==json_type_object){
                /*Simply get the array*/
                jarray = json_object_object_get(jobj, key); /*Getting the array if it is a key value pair*/

                json_object_object_foreach(jarray, key2, value2) { /*Passing through every array element*/
                    if (strcmp(key2,"start")==0)
                        (*start)=atoi(json_object_get_string(value2));
                    else if (strcmp(key2,"end")==0)
                        (*end)=atoi(json_object_get_string(value2));
                    //Free memory
                    json_object_put(value2);
                }
            }
        }
    }
    if (jobj)
        json_object_put(jobj);
}

int isAllowed(char* code, sqlite3_stmt* stmt, char** desc){
    int retval;
    const char *sched;
    int allowed;
    
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [7];
    
    int day, now, start = 0, end = 0;

    time (&rawtime);
    timeinfo = localtime (&rawtime);
    
    strftime(buffer,2,"%u",timeinfo);
    day=atoi(buffer);
    
    strftime(buffer,7,"%H%M%S",timeinfo);
    now=atoi(buffer);
    
    retval = sqlite3_bind_text(stmt,1,code,-1,SQLITE_TRANSIENT);
    if(retval){
        fprintf(stderr,"Binding statement failed\n");
        return -1;
    }

    while(1){
        // fetch a row’s status
        retval = sqlite3_step(stmt);

        // SQLITE_ROW means fetched a row
        if(retval == SQLITE_ROW){
            // sqlite3_column_text returns a const void* , typecast it to const char*
            *desc=(char*)sqlite3_column_text(stmt,0);
            allowed = sqlite3_column_int(stmt,1);
            sched = (const char*)sqlite3_column_text(stmt,2);
            
            if (allowed==0){
                return 0;
            }
            jsonparse(sched,day,&start,&end);
            if (now >= start && now <= end){
                return 1;
            }
            else{
                return 0;
            }
        }
        else if(retval == SQLITE_DONE){
            return -2;
        }
        else{
            // Some error encountered
            return -1;
        }
    }
}

void signal_handler(int signum){
    if ((signum==SIGTERM) || (signum==SIGINT) || (signum==SIGQUIT)){
        loop=0;
    }
}

int main(int argc, char **argv){
    // Create an int variable for storing the return code for each call
    int retval;
    //Will contain the badge owner's name
    char *desc;
    //Query string
    char *query, *param;
    // A prepered statement for fetching tables
    sqlite3_stmt *stmt;
    // Create a handle for database connection, create a pointer to sqlite3
    sqlite3 *handle;

    struct sigaction sig_h;
    
    //Allocate memory
    param = calloc(1,sizeof(char) * D_SIZE);
    desc = NULL;
    
    loadConf();
    
    /* Cattura segnali di uscita */
    sig_h.sa_handler=signal_handler;
    sig_h.sa_flags=0;
    /* Signals blocked during the execution of the handler. */
    sigemptyset(&sig_h.sa_mask);
    sigaddset(&sig_h.sa_mask, SIGINT);
    sigaddset(&sig_h.sa_mask, SIGTERM);
    sigaddset(&sig_h.sa_mask, SIGQUIT);
    
    sigaction(SIGQUIT,&sig_h,NULL);
    sigaction(SIGINT,&sig_h,NULL);
    sigaction(SIGTERM,&sig_h,NULL);
    
    // try to create the database. If it doesnt exist, it would be created
    // pass a pointer to the pointer to sqlite3, in short sqlite3**
    retval = sqlite3_open(dbfile,&handle);
    // If connection failed, handle returns NULL
    if(retval){
        printf("Database connection failed\n");
        exit(1);
    }
    if (verbose > 1){
        fprintf(stderr,"Connection successful\n");
    }
    
    // select rows from the table
    query = "SELECT description,allowed,sched from acl where badge_code = ?";
    retval = sqlite3_prepare_v2(handle,query,-1,&stmt,0);
    if(retval){
        fprintf(stderr,"Preparing statement failed\n");
        exit(1);
    }
    
    while (loop && fgets(param,D_SIZE,stdin)){
        //Remove trailing \n
        strtok(param,"\n");
        //Check if allowed
        retval=isAllowed(param,stmt,&desc);
        if (retval==1){
            //Allowed
            printf("Badge %s: %s - ALLOWED\n",param,desc);
            fflush(stdout);
            pin_on(doorpin);
            sleep(doortime);
            pin_off(doorpin);
        }
        else if(retval==0){
            //Denied
            printf("Badge %s: %s - DENIED\n",param,desc);
            fflush(stdout);
            if (alarmpin > 0){
                pin_on(alarmpin);
                sleep(alarmtime);
                pin_off(alarmpin);
            }
        }
        else if(retval==-2){
            //Unknown
            printf("Badge %s: UNKNOWN - DENIED BY POLICY\n",param);
            fflush(stdout);
            if (alarmpin > 0){
                pin_on(alarmpin);
                sleep(alarmtime);
                pin_off(alarmpin);
            }
        }
        else{
            printf("Internal error. Program terminated.\n");
            fflush(stdout);
            exit(1);
        }

        //Reset statement
        sqlite3_reset(stmt);
    }
    // Destroy statement
    sqlite3_finalize(stmt);
    // Close the handle to free memory
    sqlite3_close(handle);
    
    // Free all pointers
    // desc is freed automatically by sqlite
    free(param);
    free(dbfile);
    
    exit(0);
}