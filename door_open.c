#define _GNU_SOURCE
#include "common.h"
#include <pthread.h>

#define VERSION "0.3-r2"

//Debian and Gentoo (and maybe other distros) use different path for the same library
#ifdef ljson
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
int loop = 0;

char *led_on_command, *led_off_command, *door_open_command, *door_close_command, *alarm_on_command, *alarm_off_command;
#define Statusled 0
#define Door 1
#define Alarm 2

char *badge_table, *user_colname, *allowed_colname, *code_colname;
char *acl_table, *user_acl_colname, *id_device_colname, *sched_colname;
char *userdata_table, *user_userdata_colname, *name_userdata_colname, *notes_userdata_colname;

/* Debug */
short debug = 0;

#ifdef SQLITE_B
#include <sqlite3.h>

char *dbfile;
// A prepered statement for fetching tables
sqlite3_stmt *stmt;
// Create a handle for database connection, create a pointer to sqlite3
sqlite3 *handle;
#endif

#ifdef MYSQL_B
#ifdef MARIADB_B
#include <mariadb/mysql.h>
#include <mariadb/errmsg.h>
#else
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#endif

MYSQL con;
char *dbhost, *dbuser, *dbpassword, *dbname, *id;
#endif

void version() {
	printf("door_open, version %s (compiled %s, %s)\n", VERSION, __TIME__, __DATE__);
	printf("(C) 2012-2014 Gabriele Martino\n");
	printf("Website: https://github.com/pierinz/citofonoweb\n");
	printf("\nCompiled options:\n");
#ifdef json
	printf("-Djson ");
#endif
#ifdef MYSQL_B
	printf("-DMYSQL_B ");
#endif
#ifdef SQLITE_B
	printf("-DSQLITE_B ");
#endif
	printf("-DCONFPATH %s\n",
			CONFPATH);
	fflush(stdout);
}

int pin_on(short item) {
	char* command;

	switch (item) {
		case Statusled:
			command = led_on_command;
			break;
		case Door:
			command = door_open_command;
			break;
		case Alarm:
			command = alarm_on_command;
			break;
		default:
			fprintf(stderr, "Invalid value\n");
			exit(1);
			break;
	}

	if (strlen(command) < 2) {
		fprintf(stderr, "Command not specified\n");
		exit(1);
	}

	return system(command);
}

int pin_off(short item) {
	char* command;

	switch (item) {
		case Statusled:
			command = led_off_command;
			break;
		case Door:
			command = door_close_command;
			break;
		case Alarm:
			command = alarm_off_command;
			break;
		default:
			fprintf(stderr, "Invalid value\n");
			exit(1);
			break;
	}

	if (strlen(command) < 2) {
		fprintf(stderr, "Command not specified\n");
		exit(1);
	}

	return system(command);
}

void memfail() {
	perror("Cannot allocate memory");
	printf("Internal error. Program terminated.\n");
	fflush(stdout);
	if (strlen(led_off_command) > 2)
		pin_off(Statusled);
	exit(1);
}

void loadConf(char* conffile) {
	FILE* fp;
	char *line, *def = NULL, *val = NULL;
	size_t n = 1;

	fp = fopen(conffile, "r");
	if (!fp) {
		fprintf(stderr, "File %s:\n", conffile);
		perror("Error opening configuration: ");
		exit(1);
	}

	line = calloc(n + 1, sizeof (char));
	while (getline(&line, &n, fp) > 0) {
		sscanf(line, "%ms %m[^\n]", &def, &val);
		if (def == NULL)
			asprintf(&def, " ");
		if (val == NULL)
			asprintf(&val, " ");

		if (strcmp(def, "led_on_command") == 0) {
			if (asprintf(&led_on_command, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "led_off_command") == 0) {
			if (asprintf(&led_off_command, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "door_open_command") == 0) {
			if (asprintf(&door_open_command, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "door_close_command") == 0) {
			if (asprintf(&door_close_command, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "alarm_on_command") == 0) {
			if (asprintf(&alarm_on_command, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "alarm_off_command") == 0) {
			if (asprintf(&alarm_off_command, "%s", val) == -1) {
				memfail();
			}
		}
#ifdef SQLITE_B
		else if (strcmp(def, "dbfile") == 0) {
			if (asprintf(&dbfile, "%s", val) == -1) {
				memfail();
			}
		}
#endif
#ifdef MYSQL_B
		else if (strcmp(def, "dbhost") == 0) {
			if (asprintf(&dbhost, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "dbname") == 0) {
			if (asprintf(&dbname, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "dbuser") == 0) {
			if (asprintf(&dbuser, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "dbpassword") == 0) {
			if (asprintf(&dbpassword, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "id_device") == 0) {
			if (asprintf(&id, "%s", val) == -1) {
				memfail();
			}
		}
#endif
		else if (strcmp(def, "badge_table") == 0) {
			if (asprintf(&badge_table, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "user_colname") == 0) {
			if (asprintf(&user_colname, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "allowed_colname") == 0) {
			if (asprintf(&allowed_colname, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "code_colname") == 0) {
			if (asprintf(&code_colname, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "acl_table") == 0) {
			if (asprintf(&acl_table, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "id_device_colname") == 0) {
			if (asprintf(&id_device_colname, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "user_acl_colname") == 0) {
			if (asprintf(&user_acl_colname, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "sched_colname") == 0) {
			if (asprintf(&sched_colname, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "userdata_table") == 0) {
			if (asprintf(&userdata_table, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "user_userdata_colname") == 0) {
			if (asprintf(&user_userdata_colname, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "name_userdata_colname") == 0) {
			if (asprintf(&name_userdata_colname, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "notes_userdata_colname") == 0) {
			if (asprintf(&notes_userdata_colname, "%s", val) == -1) {
				memfail();
			}
		} else if (strcmp(def, "verbose") == 0) {
			verbose = atoi(val);
		} else if (strcmp(def, "doortime") == 0) {
			doortime = atof(val);
		} else if (strcmp(def, "alarmtime") == 0) {
			alarmtime = atof(val);
		}

		/* Free current values */
		free(def);
		free(val);
		def = val = NULL;
	}
	free(line);
	fclose(fp);

	if (verbose > 1) {
		fprintf(stderr, "Configuration loaded.\n");
	}
}

void jsonparse(const char* sched, int day, int* start, int* end) {
	json_object *jarray, *jobj;
	jobj = json_tokener_parse(sched);

	json_object_object_foreach(jobj, key, value) { /*Passing through every array element*/
		if (atoi(key) == day) {
			if (json_object_get_type(value) == json_type_object) {
				/*Simply get the array*/
				json_object_object_get_ex(jobj, key, &jarray); /*Getting the array if it is a key value pair*/

				json_object_object_foreach(jarray, key2, value2) { /*Passing through every array element*/
					if (strcmp(key2, "start") == 0)
						(*start) = atoi(json_object_get_string(value2));
					else if (strcmp(key2, "end") == 0)
						(*end) = atoi(json_object_get_string(value2));
					//Free memory
					if (debug > 0)
						fprintf(stderr, "Free json array element\n");
					json_object_put(value2);
				}
			}
		}
	}
	if (jobj) {
		if (debug > 0)
			fprintf(stderr, "Free json object\n");
		json_object_put(jobj);
	}
}

void allow(char* code, char* desc, char* name_u, char* note_u) {
	printf("Badge %s: %s - ALLOWED", code, desc);
	if (strlen(name_u) > 1) {
		printf(" - %s", name_u);
		if (strlen(note_u) > 1) {
			printf(" | %s", note_u);
		}
	}
	printf("\n");
	fflush(stdout);
	pin_on(Door);
	usleep(doortime * 1000000);
	pin_off(Door);
}

void deny(char* code, char* desc, char* name_u, char* note_u) {
	printf("Badge %s: %s - DENIED", code, desc);
	if (strlen(name_u) > 1) {
		printf(" - %s", name_u);
		if (strlen(note_u) > 1) {
			printf(" | %s", note_u);
		}
	}
	printf("\n");
	fflush(stdout);
	if (strlen(alarm_on_command) > 2 && strlen(alarm_off_command) > 2) {
		pin_on(Alarm);
		usleep(alarmtime * 1000000);
		pin_off(Alarm);
	}
}

void unknown(char* code) {
	printf("Badge %s: UNKNOWN - DENIED BY POLICY\n", code);
	fflush(stdout);
	if (strlen(alarm_on_command) > 2 && strlen(alarm_off_command) > 2) {
		pin_on(Alarm);
		usleep(alarmtime * 1000000);
		pin_off(Alarm);
	}
}

#ifdef SQLITE_B

void db_open() {
	// Create an int variable for storing the return code for each call
	int retval;
	// Query string
	char *query;

	// try to create the database. If it doesnt exist, it would be created
	// pass a pointer to the pointer to sqlite3, in short sqlite3**
	retval = sqlite3_open(dbfile, &handle);
	// If connection failed, handle returns NULL
	if (retval) {
		printf("Database connection failed\n");
		exit(1);
	}
	if (verbose > 1) {
		fprintf(stderr, "Connection successful\n");
	}

	// select rows from the table
	if (asprintf(&query, "SELECT %s, %s, trim(%s) from %s where %s = ?",
			user_colname, allowed_colname, sched_colname, acl_table, code_colname)) {
		perror("Cannot allocate memory");
		printf("Internal error. Program terminated.\n");
		fflush(stdout);
		if (strlen(led_off_command) > 2)
			pin_off(Statusled);
		exit(1);
	}
	retval = sqlite3_prepare_v2(handle, query, -1, &stmt, 0);
	free(query);
	if (retval) {
		printf("Preparing statement failed\n");
		exit(1);
	}
}

void db_close() {
	// Destroy statement
	sqlite3_finalize(stmt);
	// Close the handle to free memory
	sqlite3_close(handle);
	// Free all pointers
	// desc is freed automatically by sqlite
	free(dbfile);
}

int fetchRow(char* code, char** desc, int* allowed, char** sched, char** name_u, char** note_u) {
	int retval;

	retval = sqlite3_bind_text(stmt, 1, code, -1, SQLITE_TRANSIENT);
	if (retval) {
		printf("Binding statement failed\n");
		return -1;
	}

	while (1) {
		// fetch a row’s status
		retval = sqlite3_step(stmt);

		// SQLITE_ROW means fetched a row
		if (retval == SQLITE_ROW) {
			// sqlite3_column_text returns a const void* , typecast it to const char*
			*allowed = sqlite3_column_int(stmt, 1);

			asprintf(desc, "%s", sqlite3_column_text(stmt, 0));

			asprintf(sched, "%s", sqlite3_column_text(stmt, 2));

			//Reset statement
			sqlite3_reset(stmt);

			return 1;
		} else if (retval == SQLITE_DONE) {
			//Reset statement
			sqlite3_reset(stmt);

			return 0;
		} else {
			printf("Internal error. Program terminated.\n");
			fflush(stdout);
			if (strlen(led_off_command) > 2)
				pin_off(Statusled);
			exit(1);
		}
	}
}
#endif

#ifdef MYSQL_B

void db_open() {
	int reconnect = 1;
	//Retry count
	int i = 1;
	int s = 0;

	if (mysql_init(&con) == NULL) {
		printf("mysql_init() failed\n");
		exit(1);
	}

	while (mysql_real_connect(&con, dbhost, dbuser, dbpassword, dbname, 0, NULL, 0) == NULL) {
		if (i < 6) {
			printf("Connection failed #%d: %s - retry in 5s\n", i, mysql_error(&con));
			i++;
			s = sleep(5);
			while (s > 0) {
				printf("Caught signal - wait %i seconds before reconnect\n", s);
				//If interrupted by signal, sleep for the remaining time
				s = sleep(s);
			}
		} else {
			printf("Fatal error: %s\n", mysql_error(&con));
			mysql_close(&con);
			exit(1);
		}
	}

	mysql_options(&con, MYSQL_OPT_COMPRESS, 0);
	mysql_options(&con, MYSQL_OPT_RECONNECT, &reconnect);
}

void db_close() {
	mysql_close(&con);
	/* This fixes all the memory leaks */
	mysql_library_end();
}

int fetchRow(char* code, char** desc, int* allowed, char** sched, char** name_u, char** note_u) {
	MYSQL_ROW row;
	MYSQL_RES *result;
	char *query, *ujoin, *code_e;

	code_e = calloc(sizeof (char), (strlen(code)*2) + 1);
	mysql_real_escape_string(&con, code_e, code, strlen(code));

	if (userdata_table != NULL) {
		if (asprintf(&ujoin, "LEFT JOIN `%s` on `%s`.`%s`=`%s`.`%s`",
				userdata_table, userdata_table, user_userdata_colname, badge_table, user_colname) == -1) {
			memfail();
		}
	} else {
		ujoin = calloc(1, sizeof (char));
		sprintf(name_userdata_colname, sched_colname);
		sprintf(notes_userdata_colname, sched_colname);
	}

	if (asprintf(&query, "SELECT `%s`.`%s`, `%s`, trim(`%s`), `%s`, `%s` FROM `%s` "
			"LEFT JOIN `%s` on `%s`.`%s`=`%s`.`%s` and `%s`='%s' "
			"%s "
			"WHERE `%s` = '%s' ",
			badge_table, user_colname, allowed_colname, sched_colname, name_userdata_colname, notes_userdata_colname, badge_table,
			acl_table, badge_table, user_colname, acl_table, user_acl_colname, id_device_colname, id,
			ujoin,
			code_colname, code_e) == -1) {
		memfail();
	}
	free(code_e);
	free(ujoin);

	if (debug > 0)
		fprintf(stderr, "Query ready: %s\n", query);

	if (mysql_ping(&con) != 0) {
		if (mysql_errno(&con) == CR_SERVER_GONE_ERROR || mysql_errno(&con) == CR_SERVER_LOST) {
			printf("Disconnected: %s\n", mysql_error(&con));
			db_close();
			db_open();
		}
	}
	if (mysql_real_query(&con, query, strlen(query)) != 0) {
		printf("Error: %s\n", mysql_error(&con));
		db_close();
		printf("Query failed. Program terminated.\n");
		fflush(stdout);
		if (strlen(led_off_command) > 2)
			pin_off(Statusled);
		exit(1);
	}

	free(query);
	result = mysql_store_result(&con);

	if (result == NULL) {
		printf("Error: %s\n", mysql_error(&con));
		mysql_close(&con);
		printf("Internal error. Program terminated.\n");
		fflush(stdout);
		if (strlen(led_off_command) > 2)
			pin_off(Statusled);
		exit(1);
	}


	if ((row = mysql_fetch_row(result))) {
		if (debug > 0) {
			fprintf(stderr, "Query returned a row\n");
			fprintf(stderr, "Result\n");
		}

		if (asprintf(desc, "%s", row[0]) == -1) {
			memfail();
		}
		if (debug > 0)
			fprintf(stderr, "-> desc: %s\n", *desc);

		*allowed = atoi(row[1]);
		if (debug > 0)
			fprintf(stderr, "-> allowed: %d\n", *allowed);

		if (row[2]) {
			if (asprintf(sched, "%s", row[2]) == -1) {
				memfail();
			}
		} else {
			asprintf(sched, " ");
		}

		if (row[3]) {
			if (asprintf(name_u, "%s", row[3]) == -1) {
				memfail();
			}
		} else {
			asprintf(name_u, " ");
		}
		if (row[4]) {
			if (asprintf(note_u, "%s", row[4]) == -1) {
				memfail();
			}
		} else {
			asprintf(note_u, " ");
		}

		if (debug > 0) {
			fprintf(stderr, "-> sched: %s\n", *sched);
			fprintf(stderr, "Free result set...\n");
		}
		mysql_free_result(result);

		if (debug > 0)
			fprintf(stderr, "fetchRow returns 1\n");

		return 1;
	} else {
		if (debug > 0) {
			fprintf(stderr, "Query returned 0 rows\n");
			fprintf(stderr, "Free resultset...\n");
		}

		mysql_free_result(result);

		if (debug > 0)
			fprintf(stderr, "fetchRow returns 0\n");
		return 0;
	}

	return 0;
}
#endif

void isAllowed(char* code) {
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

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 2, "%u", timeinfo);
	day = atoi(buffer);

	strftime(buffer, 7, "%H%M%S", timeinfo);
	now = atoi(buffer);

	retval = fetchRow(code, &desc, &allowed, &sched, &name_u, &note_u);
	//Code not found
	if (retval < 1) {
		if (debug > 0)
			fprintf(stderr, "Code unknown\n");
		unknown(code);
	} else {
		if (debug > 0) {
			fprintf(stderr, "Code found\n");
			fprintf(stderr, "Data: %s | %d | %s\n", desc, allowed, sched);
		}

		if (allowed == 0 || strlen(sched) < 2) {
			deny(code, desc, name_u, note_u);
		} else {
			jsonparse(sched, day, &start, &end);
			if (debug > 0)
				fprintf(stderr, "Now: %d; Start: %d; End: %d\n", now, start, end);

			if (now >= start && now <= end) {
				allow(code, desc, name_u, note_u);
			} else {
				deny(code, desc, name_u, note_u);
			}
		}
		if (debug > 0)
			fprintf(stderr, "Free desc & sched...\n");

		free(desc);
		free(sched);
		free(name_u);
		free(note_u);
	}
}

void signal_handler(int signum) {
	if ((signum == SIGTERM) || (signum == SIGINT) || (signum == SIGQUIT)) {
		loop = 0;
	}
}

int main(int argc, char **argv) {
	char *param;
	struct sigaction sig_h;
	int c;
	size_t n = 1;
	char *conffile = NULL;

	/* Load settings from commandline */
	while ((c = getopt(argc, argv, "f:dhv")) != -1) {
		switch (c) {
			case 'f':
				if (asprintf(&conffile, "%s", optarg) < 0) {
					perror("asprintf");
					exit(1);
				}
				break;

			case 'd':
				debug = 1;
				break;

			case 'h':
				printf("Usage: door_open [ -f configuration file ] [ -h ]\n"
						"\n"
						"-f FILE\t\tLoad configuration from FILE (badge_daemon.conf if not specified)\n"
						"-d\t\tShow debug messages\n\n"
						"-h\t\tPrint this message and exit \n"
						"-v\t\tPrint version information and exit\n\n"
						);
				exit(1);

			case 'v':
				version();
				exit(1);
		}
	}
	if (!conffile) {
		if (asprintf(&conffile, "%s/%s", CONFPATH, "badge_daemon.conf") < 0) {
			perror("asprintf");
			exit(1);
		}
	}

	loadConf(conffile);
	free(conffile);

	/* Catch exit signals */
	sig_h.sa_handler = signal_handler;
	sig_h.sa_flags = 0;
	/* Signals blocked during the execution of the handler. */
	sigemptyset(&sig_h.sa_mask);
	sigaddset(&sig_h.sa_mask, SIGINT);
	sigaddset(&sig_h.sa_mask, SIGTERM);
	sigaddset(&sig_h.sa_mask, SIGQUIT);

	sigaction(SIGQUIT, &sig_h, NULL);
	sigaction(SIGINT, &sig_h, NULL);
	sigaction(SIGTERM, &sig_h, NULL);

	//Init database
	db_open();
	if (debug > 0)
		fprintf(stderr, "Db opened\n");

	if (strlen(led_on_command) > 2)
		pin_on(Statusled);

	if (debug > 0)
		fprintf(stderr, "Starting main loop\n");

	//There we go
	loop = 1;

	param = calloc(n, sizeof (char));
	while (loop && getline(&param, &n, stdin) > 0) {
		//Remove trailing \n
		strtok(param, "\n");

		if (debug > 0)
			fprintf(stderr, "Got param: %s\n", param);
		//Check if allowed
		isAllowed(param);
	}
	free(param);
	db_close();

	if (strlen(led_off_command) > 2)
		pin_off(Statusled);

	#ifdef MYSQL_B
	free(dbhost);
	free(dbuser);
	free(dbpassword);
	free(dbname);
	free(id);
	#endif

	free(badge_table);
	free(user_colname);
	free(allowed_colname);
	free(code_colname);
	free(acl_table);
	free(user_acl_colname);
	free(id_device_colname);
	free(sched_colname);
	free(userdata_table);
	free(user_userdata_colname);
	free(name_userdata_colname);
	free(notes_userdata_colname);

	free(led_on_command);
	free(led_off_command);
	free(door_open_command);
	free(door_close_command);
	free(alarm_on_command);
	free(alarm_off_command);

	exit(0);
}
