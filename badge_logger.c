#define _GNU_SOURCE
#include "common.h"
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <curl/curl.h>

#ifndef elsize
#define elsize 64
#endif

char* url;
char* params;
char *tmpf;

char* queue;
int qsize=1500;
int start, current, msize;
short verbose=0;
short loop=1;

void loadConf(char *conffile){
    FILE* fp;
    char line[confline], def[confdef], val[confval];

    fp=fopen(conffile,"r");
    if (!fp){
        fprintf(stderr,"File %s:\n",conffile);
        perror("Error opening configuration: ");
        exit(1);
    }

    while(fgets(line,(confline-1),fp)){
		/* Delete previous value */
		def[0]='\0';
		val[0]='\0';

        sscanf(line,"%s %[^\n]",def,val);

        if (strcmp(def,"url")==0){
            /* must be large enough to contain "val" */
            url=calloc(1,strlen(val)+1);
            strcpy(url,val);
			continue;
        }
        if (strcmp(def,"params")==0){
            /* must be large enough to contain "val" */
            params=calloc(1,strlen(val)+1);
            strcpy(params,val);
			continue;
        }
        if (strcmp(def,"queuefile")==0){
            /* must be large enough to contain "val" */
            tmpf=calloc(1,strlen(val)+1);
            strcpy(tmpf,val);
			continue;
        }
		if (strcmp(def,"queuesize")==0){
            qsize=atoi(val);
			continue;
        }
    }
    fclose(fp);

    if (verbose > 1){
        fprintf(stderr,"Configuration loaded.\n");
    }
}

int pushData(char* param){
	int qfree;

	if ((current+elsize) % (qsize*elsize) == start){
		return -1;
	}

	sprintf(queue+current,"%s",param);
	current=(current+elsize) % (qsize*elsize);

	sprintf(queue+((qsize+1)*elsize),"%d|%d",start,current);

	msync(queue,msize,MS_SYNC);
	qfree=(qsize*elsize) - abs(start - current);
	if (qfree == qsize*elsize){
		qfree=0;
	}
	return qfree;
}

int popData(char** result){
	sprintf(*result,"%s",queue+start);

	start=(start+elsize) % (qsize*elsize);
	sprintf(queue+((qsize+1)*elsize),"%d|%d",start,current);
	msync(queue,msize,MS_SYNC);

	return abs(current - start);
}
int sendData(char param[keylen]){
	char* remoteurl;
	CURL *curl;
	CURLcode res;
	int success=-1;
	long http_code=0;
	CURLcode curl_code;

	remoteurl=malloc((strlen(param)+strlen(url))*sizeof(char));
	sprintf(remoteurl,"%s%s",url,param);

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, remoteurl);

		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			success=-1;
		}
		else{
			curl_code=curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
			if (http_code == 200 && curl_code != CURLE_ABORTED_BY_CALLBACK){
				success=1;
			}
			else{
				fprintf(stderr, "The server returned an error: %li\n", http_code);
				success=-1;
			}
		}

		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}
	free(remoteurl);
	return success;
}

void emptyQueue(){
	char* element;
	int i=1;
	
	element=malloc(sizeof(char) * keylen);
	while (popData(&element) > 0){
		if (sendData(element) < 1){
			/* Stop on first error and retry in the future */
			fprintf(stderr,"%d elements sent before server vanished.\n",i);
			free(element);
			return;
		}
		i++;
	}
	free(element);
	printf("%d elements sent.\n",i);
}


int main (int argc, char *argv[]){
    //struct sigaction sig_h;
	int c,pages,fd,retry;
	char *conffile=NULL;
	char param[keylen],elem[elsize], buftime[22];
	short new=0;
	time_t rawtime;
	struct tm * timeinfo;
	
	/* Load settings from commandline */
    while ((c = getopt (argc, argv, "f:h")) != -1){
        switch (c){
            case 'f':
                if (asprintf(&conffile,"%s",optarg)<0){
					perror("asprintf");
					exit(1);
				}
                break;
            case 'h':
				printf("Usage: badge_daemon [ -f configuration file ] [ -h ]\n"
                    "\n"
                    "-f FILE\t\tLoad configuration from FILE\n"
                    "-h\t\tShow this message\n\n"
                );
                exit(1);
        }
    }
	if (!conffile){
		fprintf(stderr,"Configuration file missing.");
		exit(1);
	}

	loadConf(conffile);
	free(conffile);
	
	fd=open(tmpf, O_RDWR);
	if (fd < 0 && errno==ENOENT){
		/* First run: create file and set flag */
		fd=open(tmpf, O_RDWR | O_CREAT, S_IRWXU);
		new=1;
	}
	if (fd < 0){
		perror("fopen");
		exit(1);
	}

	/* Mmapped memory must be aligned to page size */
	pages=1+(((sizeof(char)*(qsize+1)*elsize)-1)/sysconf(_SC_PAGE_SIZE));
	msize=pages*sysconf(_SC_PAGE_SIZE);

	/* Enlarge the file to the defined size */
	if (ftruncate(fd,msize)<0){
		perror("ftruncate");
		exit(1);
	}
	/* Allocate the "shared memory" */
	queue=(char*) mmap(NULL, msize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	

	if (queue==MAP_FAILED){
		perror("mmap");
		exit(1);
	}

	if (new > 0){
		start=0;
		current=0;
	}
	else{
		fprintf(stderr,"%s\n",queue+((qsize+1)*elsize));
		sscanf(queue+((qsize+1)*elsize),"%d|%d",&start,&current);
	}

	printf("Ready to accept data.\n");
	while (loop && fgets(param,keylen,stdin)){
		//Remove trailing \n
		strtok(param,"\n");
		
		time (&rawtime);
		timeinfo = localtime (&rawtime);
		strftime(buftime,22,"%y%m%d%H%M%S",timeinfo);
		
		sprintf(elem,params,buftime,param);
		
		//if (verbose > 1)
			fprintf(stderr,"Got param: %s, index: %d %d\n",param, start, current);
		
		if (sendData(elem) < 0){
			if (pushData(elem) < 0){
				fprintf(stderr,"Too many queued elements. Waiting the server. Do not stop this program or all further elements will be lost.\n");
				retry=1;
				while (sendData(elem) < 0){
					printf("Attempt %d to contact server\n",retry);
					retry++;
					sleep(5);
				}
				printf("The server is alive! Sending all queued elements\n");
				emptyQueue();
			}
		}
		else{
			emptyQueue();
		}
		fprintf(stderr,"%d %d\n",start, current);
	}
	
	free(tmpf);
	free(url);
	free(params);
	exit(0);
}
