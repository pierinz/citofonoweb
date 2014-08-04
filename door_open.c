#define _GNU_SOURCE
#include "common.h"
#include <pthread.h>

//Debian and Gentoo (and maybe other distros) use different path for the same library
#ifdef json
	#include <json/json.h>
#else
	#include <json-c/json.h>
#endif

#ifndef CONFPATH
	#define CONFPATH "conf"
#endif

#if !defined(MYSQL_B) && !defined(SQLITE_B)
#define SQLITE_B
#endif

int verbose;
float doortime, alarmtime;
int loop=1;

char *led_on_command, *led_off_command, *door_open_command, *door_close_command, *alarm_on_command, *alarm_off_command;
#define Statusled 0
#define Door 1
#define Alarm 2

char *badge_table, *user_colname, *allowed_colname, *code_colname;
char *acl_table, *user_acl_colname, *id_device_colname, *sched_colname;
char *userdata_table, *user_userdata_colname, *name_userdata_colname, *notes_userdata_colname;

/* Debug */
short debug=0;

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
char *dbhost, *dbuser, *dbpassword, *dbname, *id;
#endif

void loadConf(char* conffile){
	FILE* fp;
	char line[confline+1], def[confdef], val[confval];

	fp=fopen(conffile, "r");
	if (!fp){
		fprintf(stderr,"File %s:\n",conffile);
		perror("Error opening configuration: ");
		exit(1);
	}
	while(fgets(line,confline,fp)){
		/* Delete previous value */
		def[0]='\0';
		val[0]='\0';
		sscanf(line,"%s %[^\n]",def,val);
		if (strcmp(def,"led_on_command")==0){
			/* must be large enough to contain "val" */
			led_on_command=calloc(1,strlen(val)+1);
			sprintf(led_on_command, "%s", val);
			continue;
		}
		if (strcmp(def,"led_off_command")==0){
			/* must be large enough to contain "val" */
			led_off_command=calloc(1,strlen(val)+1);
			sprintf(led_off_command, "%s", val);
			continue;
		}
		if (strcmp(def,"door_open_command")==0){
			/* must be large enough to contain "val" */
			door_open_command=calloc(1,strlen(val)+1);
			sprintf(door_open_command, "%s", val);
			continue;
		}
		if (strcmp(def,"door_close_command")==0){
			/* must be large enough to contain "val" */
			door_close_command=calloc(1,strlen(val)+1);
			sprintf(door_close_command, "%s", val);
			continue;
		}
		if (strcmp(def,"alarm_on_command")==0){
			/* must be large enough to contain "val" */
			alarm_on_command=calloc(1,strlen(val)+1);
			sprintf(alarm_on_command, "%s", val);
			continue;
		}
		if (strcmp(def,"alarm_off_command")==0){
			/* must be large enough to contain "val" */
			alarm_off_command=calloc(1,strlen(val)+1);
			sprintf(alarm_off_command, "%s", val);
			continue;
		}
		#ifdef SQLITE_B
		if (strcmp(def,"dbfile")==0){
			/* must be large enough to contain "val" */
			dbfile=calloc(1,strlen(val)+1);
			strcpy(dbfile,val);
		}
		#endif
		#ifdef MYSQL_B
		if (strcmp(def,"dbhost")==0){
			/* must be large enough to contain "val" */
			dbhost=calloc(1,strlen(val)+1);
			strcpy(dbhost,val);
			continue;
		}
		if (strcmp(def,"dbname")==0){
			/* must be large enough to contain "val" */
			dbname=calloc(1,strlen(val)+1);
			strcpy(dbname,val);
			continue;
		}
		if (strcmp(def,"dbuser")==0){
			/* must be large enough to contain "val" */
			dbuser=calloc(1,strlen(val)+1);
			strcpy(dbuser,val);
			continue;
		}
		if (strcmp(def,"dbpassword")==0){
			/* must be large enough to contain "val" */
			dbpassword=calloc(1,strlen(val)+1);
			strcpy(dbpassword,val);
			continue;
		}
		if (strcmp(def,"id_device")==0){
			/* must be large enough to contain "val" */
			id=calloc(1,strlen(val)+1);
			strcpy(id,val);
			continue;
		}
		#endif

		if (strcmp(def,"badge_table")==0){
			/* must be large enough to contain "val" */
			badge_table=calloc(1,strlen(val)+1);
			strcpy(badge_table,val);
			continue;
		}
		if (strcmp(def,"user_colname")==0){
			/* must be large enough to contain "val" */
			user_colname=calloc(1,strlen(val)+1);
			strcpy(user_colname,val);
			continue;
		}
		if (strcmp(def,"allowed_colname")==0){
			/* must be large enough to contain "val" */
			allowed_colname=calloc(1,strlen(val)+1);
			strcpy(allowed_colname,val);
			continue;
		}
		if (strcmp(def,"code_colname")==0){
			/* must be large enough to contain "val" */
			code_colname=calloc(1,strlen(val)+1);
			strcpy(code_colname,val);
			continue;
		}
		if (strcmp(def,"acl_table")==0){
			/* must be large enough to contain "val" */
			acl_table=calloc(1,strlen(val)+1);
			strcpy(acl_table,val);
			continue;
		}
		if (strcmp(def,"id_device_colname")==0){
			/* must be large enough to contain "val" */
			id_device_colname=calloc(1,strlen(val)+1);
			strcpy(id_device_colname,val);
			continue;
		}
		if (strcmp(def,"user_acl_colname")==0){
			/* must be large enough to contain "val" */
			user_acl_colname=calloc(1,strlen(val)+1);
			strcpy(user_acl_colname,val);
			continue;
		}
		if (strcmp(def,"sched_colname")==0){
			/* must be large enough to contain "val" */
			sched_colname=calloc(1,strlen(val)+1);
			strcpy(sched_colname,val);
			continue;
		}
		if (strcmp(def,"userdata_table")==0){
			/* must be large enough to contain "val" */
			userdata_table=calloc(1,strlen(val)+1);
			strcpy(userdata_table,val);
			continue;
		}
		if (strcmp(def,"user_userdata_colname")==0){
			/* must be large enough to contain "val" */
			user_userdata_colname=calloc(1,strlen(val)+1);
			strcpy(user_userdata_colname,val);
			continue;
		}
		if (strcmp(def,"name_userdata_colname")==0){
			/* must be large enough to contain "val" */
			name_userdata_colname=calloc(1,strlen(val)+1);
			strcpy(name_userdata_colname,val);
			continue;
		}
		if (strcmp(def,"notes_userdata_colname")==0){
			/* must be large enough to contain "val" */
			notes_userdata_colname=calloc(1,strlen(val)+1);
			strcpy(notes_userdata_colname,val);
			continue;
		}
		
		if (strcmp(def,"verbose")==0){
			verbose=atoi(val);
			continue;
		}
		if (strcmp(def,"doortime")==0){
			doortime=atof(val);
			continue;
		}
		if (strcmp(def,"alarmtime")==0){
			alarmtime=atof(val);
			continue;
		}
	}
	fclose(fp);

	if (verbose > 1){
		fprintf(stderr,"Configuration loaded.\n");
	}
}

void pin_on(short item){
	char* command;
	
	switch (item){
		case Statusled:
			command=led_on_command;
		break;
		case Door:
			command=door_open_command;
		break;
		case Alarm:
			command=alarm_on_command;
		break;
		default:
			fprintf(stderr,"Invalid value\n");
			exit(1);
		break;
	}
	
	if (strlen(command)<2){
		fprintf(stderr,"Command not specified\n");
		exit(1);
	}
	
	system(command);
}

void pin_off(short item){
	char* command;
	
	switch (item){
		case Statusled:
			command=led_off_command;
		break;
		case Door:
			command=door_close_command;
		break;
		case Alarm:
			command=alarm_off_command;
		break;
		default:
			fprintf(stderr,"Invalid value\n");
			exit(1);
		break;
	}
	
	if (strlen(command)<2){
		fprintf(stderr,"Command not specified\n");
		exit(1);
	}
	
	system(command);
}


void jsonparse(const char* sched, int day, int* start, int* end){
	json_object *jarray, *jobj;
	jobj = json_tokener_parse(sched);

	json_object_object_foreach(jobj, key, value) { /*Passing through every array element*/
		if (atoi(key)==day){
			if (json_object_get_type(value)==json_type_object){
				/*Simply get the array*/
				json_object_object_get_ex(jobj, key, &jarray); /*Getting the array if it is a key value pair*/

				json_object_object_foreach(jarray, key2, value2) { /*Passing through every array element*/
					if (strcmp(key2,"start")==0)
						(*start)=atoi(json_object_get_string(value2));
					else if (strcmp(key2,"end")==0)
						(*end)=atoi(json_object_get_string(value2));
					//Free memory
					if (debug > 0)
						fprintf(stderr,"Free json array element\n");
					json_object_put(value2);
				}
			}
		}
	}
	if (jobj){
		if (debug > 0)
			fprintf(stderr,"Free json object\n");
		json_object_put(jobj);
	}
}

void allow(char* code, char* desc, char* name_u, char* note_u){
	printf("Badge %s: %s - ALLOWED", code, desc);
	if (strlen(name_u)>1){
		printf(" - %s", name_u);
		if (strlen(note_u)>1){
			printf(" | %s", note_u);
		}
	}
	printf("\n");
	fflush(stdout);
	pin_on(Door);
	usleep(doortime*1000000);
	pin_off(Door);
}

void deny(char* code, char* desc, char* name_u, char* note_u){
	printf("Badge %s: %s - DENIED", code, desc);
	if (strlen(name_u)>1){
		printf(" - %s", name_u);
		if (strlen(note_u)>1){
			printf(" | %s", note_u);
		}
	}
	printf("\n");
	fflush(stdout);
	if (strlen(alarm_on_command) > 2 && strlen(alarm_off_command) > 2){
		pin_on(Alarm);
		usleep(alarmtime*1000000);
		pin_off(Alarm);
	}
}

void unknown(char* code){
	printf("Badge %s: UNKNOWN - DENIED BY POLICY\n",code);
	fflush(stdout);
	if (strlen(alarm_on_command) > 2 && strlen(alarm_off_command) > 2){
		pin_on(Alarm);
		usleep(alarmtime*1000000);
		pin_off(Alarm);
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
	asprintf(&query, "SELECT %s, %s, trim(%s) from %s where %s = ?",
			user_colname, allowed_colname, sched_colname, acl_table, code_colname);
	retval = sqlite3_prepare_v2(handle,query,-1,&stmt,0);
	free(query);
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

int fetchRow(char* code, char** desc, int* allowed, char** sched, char** name_u, char** note_u){
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
			if (strlen(led_off_command)>2)
				pin_off(Statusled);
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

int fetchRow(char* code, char** desc, int* allowed, char** sched, char** name_u, char** note_u){
	MYSQL_ROW row;
	MYSQL_RES *result;
	char *query, *ujoin, *code_e;

	code_e=calloc(sizeof(char), (strlen(code)*2)+1);
	mysql_real_escape_string(con, code_e, code, strlen(code));

	if (userdata_table != NULL){
		if (asprintf(&ujoin, "LEFT JOIN `%s` on `%s`.`%s`=`%s`.`%s`",
				userdata_table, userdata_table, user_userdata_colname, badge_table, user_colname)==-1){
			perror("Cannot allocate memory");
			printf("Internal error. Program terminated.\n");
			fflush(stdout);
			if (strlen(led_off_command)>2)
				pin_off(Statusled);
			exit(1);
		}
	}
	else{
		ujoin=calloc(1,sizeof(char));
		sprintf(name_userdata_colname,sched_colname);
		sprintf(notes_userdata_colname,sched_colname);
	}
	
	if (asprintf(&query, "SELECT `%s`.`%s`, `%s`, trim(`%s`), `%s`, `%s` FROM `%s` "
			"LEFT JOIN `%s` on `%s`.`%s`=`%s`.`%s` and `%s`='%s' "
			"%s "
			"WHERE `%s` = '%s' ",
			badge_table, user_colname,allowed_colname,sched_colname,name_userdata_colname,notes_userdata_colname,badge_table,
			acl_table, badge_table, user_colname, acl_table, user_acl_colname, id_device_colname,id,
			ujoin,
			code_colname,code_e)==-1){
		perror("Cannot allocate memory");
		printf("Internal error. Program terminated.\n");
		fflush(stdout);
		if (strlen(led_off_command)>2)
			pin_off(Statusled);
		exit(1);
	}
	free(code_e);
	free(ujoin);

	if (debug > 0)
		fprintf(stderr,"Query ready: %s\n",query);

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
		if (strlen(led_off_command)>2)
			pin_off(Statusled);
		exit(1);
	}

	free(query);
	result = mysql_use_result(con);

	if (result == NULL){
		printf("Error: %s\n", mysql_error(con));
		mysql_close(con);
		printf("Internal error. Program terminated.\n");
		fflush(stdout);
		if (strlen(led_off_command)>2)
			pin_off(Statusled);
		exit(1);
	}

	if ((row = mysql_fetch_row(result))){
		if (debug > 0){
			fprintf(stderr,"Query returned a row\n");
			fprintf(stderr,"Result\n");
		}
	
		*allowed=atoi(row[1]);
		if (debug > 0)
			fprintf(stderr,"-> allowed: %d\n",*allowed);

		*desc=calloc(sizeof(char), strlen(row[0]));
		sprintf(*desc,"%s",row[0]);
		if (debug > 0)
			fprintf(stderr,"-> desc: %s\n",*desc);

		if (row[2]){
			*sched=calloc(sizeof(char), strlen(row[2]));
			sprintf(*sched,"%s",row[2]);
		}
		else{
			*sched=calloc(sizeof(char), 2);
			sprintf(*sched," ");
		}
		
		if (row[3]){
			*name_u=calloc(sizeof(char), strlen(row[3]));
			sprintf(*name_u,"%s",row[3]);
		}
		else{
			*name_u=calloc(sizeof(char), 2);
			sprintf(*name_u," ");
		}
		if (row[4]){
			*note_u=calloc(sizeof(char), strlen(row[4]));
			sprintf(*note_u,"%s",row[4]);
		}
		else{
			*note_u=calloc(sizeof(char), 2);
			sprintf(*note_u," ");
		}
		
		if (debug > 0){
			fprintf(stderr,"-> sched: %s\n",*sched);
			fprintf(stderr,"Free result set...\n");
		}
		mysql_free_result(result);
		
		if (debug > 0)
			fprintf(stderr,"fetchRow returns 1\n");
		
		return 1;
	}
	else{
		if (debug > 0){
			fprintf(stderr,"Query returned 0 rows\n");
			fprintf(stderr,"Free resultset...\n");
		}

		mysql_free_result(result);
		
		if (debug > 0)
			fprintf(stderr,"fetchRow returns 0\n");
		return 0;
	}
	
	return 0;
}
#endif

void isAllowed(char* code){
	int retval;
	char *sched;
	//Will contain the badge owner's code
	char *desc;
	//Owner's name
	char *name_u;
	//Additional notes about the owner
	char *note_u;
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

	retval=fetchRow(code,&desc,&allowed,&sched,&name_u,&note_u);
	//Code not found
	if (retval < 1){
		if (debug > 0)
			fprintf(stderr,"Code unknown\n");
		unknown(code);
	}
	else{
		if (debug > 0){
			fprintf(stderr,"Code found\n");
			fprintf(stderr,"Data: %s | %d | %s\n",desc, allowed, sched);
		}
		
		if (allowed==0 || strlen(sched)<2 ){
			deny(code, desc, name_u, note_u);
		}
		else{
			jsonparse(sched,day,&start,&end);
			if (debug > 0)
				fprintf(stderr,"Now: %d; Start: %d; End: %d\n",now, start, end);
			
			if (now >= start && now <= end){
				allow(code, desc, name_u, note_u);
			}
			else{
				deny(code, desc, name_u, note_u);
			}
		}
		if (debug > 0)
				fprintf(stderr,"Free desc & sched...\n");
		
		free(desc);
		free(sched);
		free(name_u);
		free(note_u);
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
	int c;
	char *conffile=NULL;
	
	/* Load settings from commandline */
	while ((c = getopt (argc, argv, "f:dh")) != -1){
		switch (c){
			case 'f':
				if (asprintf(&conffile,"%s",optarg)<0){
					perror("asprintf");
					exit(1);
				}
			break;
			
			case 'd':
				debug=1;
			break;
			
			case 'h':
				printf("Usage: door_open [ -f configuration file ] [ -h ]\n"
					"\n"
					"-f FILE\t\tLoad configuration from FILE (badge_daemon.conf if not specified)\n"
					"-d\t\tShow debug messages\n\n"
					"-h\t\tShow this message\n\n"
				);
				exit (1);
			break;
		}
	}
	if (!conffile){
		if (asprintf(&conffile,"%s/%s",CONFPATH,"badge_daemon.conf")<0){
			perror("asprintf");
			exit(1);
		}
	}
	
	loadConf(conffile);
	free(conffile);
	
	//Allocate memory
	param = calloc(1,sizeof(char) * keylen);

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
	if (debug > 0)
		fprintf(stderr,"Db opened\n");

	if (strlen(led_on_command)>2)
		pin_on(Statusled);

	if (debug > 0)
		fprintf(stderr,"Starting main loop\n");
	while (loop && fgets(param,keylen,stdin)){
		//Remove trailing \n
		strtok(param,"\n");
		
		if (debug > 0)
			fprintf(stderr,"Got param: %s\n",param);
		//Check if allowed
		isAllowed(param);
	}
	db_close();

	// Free all pointers
	free(param);

	if (strlen(led_off_command)>2)
		pin_off(Statusled);

	exit(0);
}
