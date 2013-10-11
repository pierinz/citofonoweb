#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <dlfcn.h>

//Debian and Gentoo (and maybe other distros) use different path for the same library
#ifdef json
    #include <json/json.h>
#else
    #include <json-c/json.h>
#endif

#ifndef CONFPATH
    #define CONFPATH "conf/badge_daemon.conf"
#endif

#ifndef D_SIZE
    #define D_SIZE 40
#endif

#if !defined(MYSQL_B) && !defined(SQLITE_B)
#define SQLITE_B
#endif

int verbose, doorpin, doortime, alarmpin, alarmtime;
short light=0;
short statusled=17;
int loop=1;

char* libdoor;

//Libdoor functions
void *lib_handle;
void (*pin_on)(short pin),(*pin_off)(short pin), (*pin_init)(), (*pin_clean)();

#ifdef SQLITE_B
#include <sqlite3.h>

char *dbfile;
// A prepered statement for fetching tables
sqlite3_stmt *stmt;
// Create a handle for database connection, create a pointer to sqlite3
sqlite3 *handle;
#endif

#ifdef MYSQL_B
#include <mysql/mysql.h>
#include <mysql/errmsg.h>

MYSQL *con;
char *dbhost, *dbuser, *dbpassword, *dbname, *id, *code_colname;
#endif

void loadConf(){
    FILE* fp;
    char line[255],def[55],val[200];
    char *error;
    
    fp=fopen(CONFPATH, "r");
    if (!fp){
        fprintf(stderr,"File %s:\n",CONFPATH);
        perror("Error opening configuration: ");
        exit(1);
    }
    while(fgets(line,255,fp)){
        sscanf(line,"%s%s",def,val);
        if (strcmp(def,"libdoor")==0){
            /* must be large enough to contain "val" */
            libdoor=calloc(1,sizeof(val));
            strcpy(libdoor,val);
            lib_handle=dlopen(libdoor,RTLD_NOW);
            if (!lib_handle) {
                printf("Error loading library: %s\n",dlerror());
                exit(1);
            }
            pin_init = dlsym(lib_handle, "pin_init");
            if ((error = dlerror()) != NULL)  {
                printf("Error loading library: %s\n",error);
                exit(1);
            }
            pin_on = dlsym(lib_handle, "pin_on");
            if ((error = dlerror()) != NULL)  {
                printf("Error loading library: %s\n",error);
                exit(1);
            }
            pin_off = dlsym(lib_handle, "pin_off");
            if ((error = dlerror()) != NULL)  {
                printf("Error loading library: %s\n",error);
                exit(1);
            }
            pin_clean = dlsym(lib_handle, "pin_clean");
            if ((error = dlerror()) != NULL)  {
                printf("Error loading library: %s\n",error);
                exit(1);
            }
        }
        #ifdef SQLITE_B
        if (strcmp(def,"dbfile")==0){
            /* must be large enough to contain "val" */
            dbfile=calloc(1,sizeof(val));
            strcpy(dbfile,val);
        }
        #endif
        #ifdef MYSQL_B
        if (strcmp(def,"dbhost")==0){
            /* must be large enough to contain "val" */
            dbhost=calloc(1,sizeof(val));
            strcpy(dbhost,val);
        }
        if (strcmp(def,"dbname")==0){
            /* must be large enough to contain "val" */
            dbname=calloc(1,sizeof(val));
            strcpy(dbname,val);
        }
        if (strcmp(def,"dbuser")==0){
            /* must be large enough to contain "val" */
            dbuser=calloc(1,sizeof(val));
            strcpy(dbuser,val);
        }
        if (strcmp(def,"dbpassword")==0){
            /* must be large enough to contain "val" */
            dbpassword=calloc(1,sizeof(val));
            strcpy(dbpassword,val);
        }
        if (strcmp(def,"id_device")==0){
            /* must be large enough to contain "val" */
            id=calloc(1,sizeof(val));
            strcpy(id,val);
        }
        if (strcmp(def,"code_colname")==0){
            /* must be large enough to contain "val" */
            code_colname=calloc(1,sizeof(val));
            strcpy(code_colname,val);
        }
        #endif
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
        if (strcmp(def,"light")==0){
            light=atoi(val);
        }
        if (strcmp(def,"statusled")==0){
            statusled=atoi(val);
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

void allow(char* code, char* desc){
    printf("Badge %s: %s - ALLOWED\n",code,desc);
    fflush(stdout);
    (*pin_on)(doorpin);
    sleep(doortime);
    (*pin_off)(doorpin);
}

void deny(char* code, char* desc){
    printf("Badge %s: %s - DENIED\n",code,desc);
    fflush(stdout);
    if (alarmpin > 0){
        (*pin_on)(alarmpin);
        sleep(alarmtime);
        (*pin_off)(alarmpin);
    }
}

void unknown(char* code){
    printf("Badge %s: UNKNOWN - DENIED BY POLICY\n",code);
    fflush(stdout);
    if (alarmpin > 0){
        (*pin_on)(alarmpin);
        sleep(alarmtime);
        (*pin_off)(alarmpin);
    }
}

#ifdef SQLITE_B
void db_open(){
    // Create an int variable for storing the return code for each call
    int retval;
    // Query string
    char *query;
    
    // try to create the database. If it doesnt exist, it would be created
    // pass a pointer to the pointer to sqlite3, in short sqlite3**
    retval = sqlite3_open(dbfile, &handle);
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
        printf("Preparing statement failed\n");
        exit(1);
    }
}

void db_close(){
    // Destroy statement
    sqlite3_finalize(stmt);
    // Close the handle to free memory
    sqlite3_close(handle);
    // Free all pointers
    // desc is freed automatically by sqlite
    free(dbfile);
}

int fetchRow(char* code, char** desc, int* allowed, char** sched){
    int retval;
    
    retval = sqlite3_bind_text(stmt,1,code,-1,SQLITE_TRANSIENT);
    if(retval){
        printf("Binding statement failed\n");
        return -1;
    }

    while(1){
        // fetch a row’s status
        retval = sqlite3_step(stmt);

        // SQLITE_ROW means fetched a row
        if(retval == SQLITE_ROW){
            // sqlite3_column_text returns a const void* , typecast it to const char*
            *allowed= sqlite3_column_int(stmt,1);
            
            *desc=calloc(sizeof(char), strlen((char*)sqlite3_column_text(stmt,0)));
            sprintf(*desc,"%s",sqlite3_column_text(stmt,0));
            
            *sched=calloc(sizeof(char), strlen((char*)sqlite3_column_text(stmt,2)));
            sprintf(*sched,"%s",sqlite3_column_text(stmt,2));
            
            //Reset statement
            sqlite3_reset(stmt);
            
            return 1;
        }
        else if(retval == SQLITE_DONE){
            //Reset statement
            sqlite3_reset(stmt);
            
            return 0;
        }
        else{
            printf("Internal error. Program terminated.\n");
            fflush(stdout);
            if (light)
                (*pin_off)(statusled);
            (*pin_clean)();
            exit(1);
        }
    }
}
#endif

#ifdef MYSQL_B
void db_open(){
    con = mysql_init(NULL);
    int reconnect=1;
    //Retry count
    int i = 1;
  
    if (con == NULL){
        printf("mysql_init() failed\n");
        exit(1);
    }  

    while (mysql_real_connect(con, dbhost, dbuser, dbpassword, dbname, 0, NULL, 0) == NULL){
        if (i < 6){
            printf("Connection failed #%d: %s - retry in 5s\n",i,mysql_error(con));
            i++;
            sleep(5);
        }
        else{
            printf("Fatal error: %s\n", mysql_error(con));
            mysql_close(con);
            exit(1);
        }
    }
    
    
    mysql_options(con,MYSQL_OPT_COMPRESS,0);
    mysql_options(con,MYSQL_OPT_RECONNECT,&reconnect);
}

void db_close(){
    mysql_close(con);
}

int fetchRow(char* code, char** desc, int* allowed, char** sched){
    MYSQL_ROW row;
    MYSQL_RES *result;
    char *query, *code_e;
    
    code_e=calloc(sizeof char, strlen(code)*2+1);
    mysql_real_escape_string(con, code_e, code, strlen(code));
    
    if (asprintf(&query,"SELECT `users`.`user`, allowed, sched FROM `users` LEFT JOIN `acl` on `users`.user=acl.user and id_device='%s' WHERE `%s` = '%s' ",id,code_colname,code)==-1){
        perror("Cannot allocate memory");
        printf("Internal error. Program terminated.\n");
        fflush(stdout);
        if (light)
            (*pin_off)(statusled);
        (*pin_clean)();
        exit(1);
    }
    free(code_e);
    
    if (mysql_ping(con)!=0){
        if (mysql_errno(con)==CR_SERVER_GONE_ERROR || mysql_errno(con)==CR_SERVER_LOST){
            printf("Disconnected: %s\n", mysql_error(con));
            db_close();
            db_open();
        }
    }
    if (mysql_real_query(con, query, strlen(query))!=0){
        printf("Error: %s\n", mysql_error(con));
        db_close();
        printf("Query failed. Program terminated.\n");
        fflush(stdout);
        if (light)
            (*pin_off)(statusled);
        (*pin_clean)();
        exit(1);   
    }
    
    free(query);
    result = mysql_store_result(con);

    if (result == NULL){
        printf("Error: %s\n", mysql_error(con));
        mysql_close(con);
        printf("Internal error. Program terminated.\n");
        fflush(stdout);
        if (light)
            (*pin_off)(statusled);
        (*pin_clean)();
        exit(1);
    }

    if (mysql_num_rows(result) == 0){
        mysql_free_result(result);
        return 0;
    }

    while ((row = mysql_fetch_row(result))){ 
        *allowed=atoi(row[1]);
        
        *desc=calloc(sizeof(char), strlen(row[0]));
        sprintf(*desc,"%s",row[0]);

        if (row[2]){
            *sched=calloc(sizeof(char), strlen(row[2]));
            sprintf(*sched,"%s",row[2]);
        }
        else{
            *sched=calloc(sizeof(char), 2);
            sprintf(*sched," ");
        }
        
        mysql_free_result(result);
        return 1;
    }
    return 0;
}
#endif

void isAllowed(char* code){
    int retval;
    char *sched;
    //Will contain the badge owner's name
    char *desc;
    int allowed;
        
    desc = NULL;
    
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
    
    retval=fetchRow(code,&desc,&allowed,&sched);
    //Code not found
    if (retval < 1){
        unknown(code);
    }
    else{
        if (allowed==0 || strlen(sched)<2 ){
            deny(code, desc);
        }
        else{
            jsonparse(sched,day,&start,&end);
            if (now >= start && now <= end){
                allow(code, desc);
            }
            else{
                deny(code, desc);
            }
        }
        free(desc);
        free(sched);
    }
}

void signal_handler(int signum){
    if ((signum==SIGTERM) || (signum==SIGINT) || (signum==SIGQUIT)){
        loop=0;
    }
}

int main(int argc, char **argv){
    char *param;
    struct sigaction sig_h;
    
    //Allocate memory
    param = calloc(1,sizeof(char) * D_SIZE);
    
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
    
    //Init database
    db_open();
    
    (*pin_init)();
    
    if (light)
        (*pin_on)(statusled);
    
    while (loop && fgets(param,D_SIZE,stdin)){
        //Remove trailing \n
        strtok(param,"\n");
        //Check if allowed
        isAllowed(param);
    }
    db_close();
    
    // Free all pointers
    free(param);
    
    if (light)
        (*pin_off)(statusled);
    
    (*pin_clean)();
    
    dlclose(lib_handle);
    exit(0);
}