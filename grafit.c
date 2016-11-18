
/*********************************************************
*
* grafit.c - NEB module for sending performance data
*  to a graphit server
*
*	ehl0 (Laszlo Kollar)
*    http://kollarlaszlo.hu
*    daddy@kollarlaszlo.hu)
*
* Compile with the following command:
*			gcc -fPic -shared -o grafit.o utils.c grafit.c
*
***********************************************************/

/* include definitions */
#include "grafit.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "include/nebmodules.h"
#include "include/nebcallbacks.h"
#include "include/nebstructs.h"
#include "include/broker.h"
#include "include/config.h"
#include "include/common.h"
#include "include/icinga.h"

#include "utils.h"

NEB_API_VERSION(CURRENT_NEB_API_VERSION);

extern int process_performance_data;
void 	*grafit_module_handle = NULL;
char 	*graphite_host=NULL;
int		graphite_port=2003;
char 	*grafit_prefix=NULL;
int 	graphite_ok=FALSE;
int 	graphite_socket;
static struct sockaddr_in serv_addr; 

int grafit_handle_data(int, void *);

int nebmodule_init(int flags, char *args, nebmodule *handle) {
	char temp_buffer[2048];
	time_t current_time;
	unsigned long interval;

	grafit_module_handle = handle;

	neb_set_module_info(grafit_module_handle, NEBMODULE_MODINFO_TITLE, "grafit");
	neb_set_module_info(grafit_module_handle, NEBMODULE_MODINFO_AUTHOR, "Laszlo Kollar");
	neb_set_module_info(grafit_module_handle, NEBMODULE_MODINFO_TITLE, "Graphite module");
	neb_set_module_info(grafit_module_handle, NEBMODULE_MODINFO_VERSION, "1.2");
	neb_set_module_info(grafit_module_handle, NEBMODULE_MODINFO_LICENSE, "GPL v2");
	neb_set_module_info(grafit_module_handle, NEBMODULE_MODINFO_DESC, "Nagios/Icinga NEB module -- sends perfdata to graphite.");

	if (process_performance_data == FALSE) {
		write_to_all_logs("grafit: Grafit cannot work without enabled performance data in icinga.cfg/nagios.cfg.", NSLOG_INFO_MESSAGE);
		write_to_all_logs("grafit: Please enable it with 'process_performance_data=1' in icinga.cfg/nagios.cfg", NSLOG_INFO_MESSAGE);
		return -1;
	}
	else{
		write_to_all_logs("grafit: Performance data processing enabled..", NSLOG_INFO_MESSAGE);
	}

	if (grafit_process_module_args(args) == ERROR) {
		write_to_all_logs("grafit: An error occurred while attempting to process module arguments.", NSLOG_INFO_MESSAGE);
		return -1;
	}
	else{
		if(graphite_host==NULL)
			graphite_host=strdup("127.0.0.1");

		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "grafit: Graphite host: %s:%d",graphite_host,graphite_port);
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		write_to_all_logs(temp_buffer, NSLOG_INFO_MESSAGE);	}
		write_to_all_logs("grafit: module successfully initialized!", NSLOG_INFO_MESSAGE);

	neb_register_callback(NEBCALLBACK_SERVICE_CHECK_DATA, grafit_module_handle, 0, grafit_handle_data);
	return 0;
	}

int nebmodule_deinit(int flags, int reason) {
	char temp_buffer[1024];
	int socket;

	neb_deregister_callback(NEBCALLBACK_SERVICE_CHECK_DATA, grafit_handle_data);

	if(grafit_prefix) free(grafit_prefix);
	if(graphite_host) free(graphite_host);

	snprintf(temp_buffer, sizeof(temp_buffer) - 1, "grafit: Graphite module deinitialized!\n");
	temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
	write_to_all_logs(temp_buffer, NSLOG_INFO_MESSAGE);
	return 0;
	}


int grafit_handle_data(int event_type, void *data) {
  nebstruct_host_check_data *hostchkdata =NULL;
  nebstruct_service_check_data *srvchkdata = NULL;
	char temp_buffer[MAXBUF];
	char *perfd = NULL;
	char *srv_desc = NULL;
	char *metric = NULL;
	char **tokens = NULL;
	char **token_head = NULL;
	char **perftokens = NULL;
	char **perfitems =NULL;
	time_t tstamp = time(NULL);

	if(event_type==NEBCALLBACK_SERVICE_CHECK_DATA){
		if((srvchkdata = (nebstruct_service_check_data *)data)) {
 	  	if (srvchkdata->type == NEBTYPE_SERVICECHECK_PROCESSED && srvchkdata->perf_data != NULL) {
			if(graphite_init(graphite_host,graphite_port)==0)
					graphite_ok=TRUE;
				else
					graphite_ok=FALSE;
				srv_desc = strdup(srvchkdata->service_description);
				replace_char(srv_desc,' ','_');
				replace_char(srv_desc,'.','_');

				perfd = strdup(srvchkdata->perf_data);
				remchar(perfd,'\n');
				parse_quote(perfd,'\'');
				tokens = split(perfd, ' ');
				token_head=tokens;
				while(*tokens!=NULL)
				{
					remchar(*tokens,'\'');
					if(*tokens!=NULL || *tokens != '\0'){
						perftokens=split(*tokens,'=');
						if(perftokens!=NULL && perftokens[1]!=NULL ){
							perfitems=split(perftokens[1],';');
							if(perfitems!=NULL && perfitems[0]!=NULL){
								metric=strip_metric(perfitems[0]);
								if(metric){
									if(graphite_ok==FALSE)
									{
										graphite_disconnect();
										graphite_init(graphite_host,graphite_port);
									}
									snprintf(temp_buffer, sizeof(temp_buffer) - 1, "%s.%s.%s.%s.%s %s %lu\n",
											grafit_prefix,"service",srvchkdata->host_name,
											srv_desc,perftokens[0],metric,tstamp);
									temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
									//write_to_all_logs(temp_buffer, NSLOG_INFO_MESSAGE);
									if(write(graphite_socket,temp_buffer,strlen(temp_buffer)) < 0)
										fprintf(stderr, "error while sending request to server\n");
									bzero(temp_buffer,MAXBUF);
									*tokens++;
									free(metric);
									freetokens(perfitems);
									freetokens(perftokens);
								}else{
									fprintf(stderr," metric NULL!: %s\n",srvchkdata->perf_data );
									freetokens(perfitems);
									freetokens(perftokens);
									break;
								}
							}else{
								fprintf(stderr,"perfitems[0] NULL!: %s\n",srvchkdata->perf_data );
								freetokens(perftokens);
								break;
							}
						}else{
							fprintf(stderr,"perftokens[1] NULL!: %s\n",srvchkdata->perf_data );
							break;
						}
					}else{
						fprintf(stderr,"*tokens NULL!: %s\n",srvchkdata->perf_data );
						break;
					}
				}	
				if(token_head) freetokens(token_head);
				free(srv_desc);
				if(perfd) free(perfd);
				graphite_disconnect(); 
			}
		}
	}
	return 0;
}

void graphite_disconnect()
{
	if (graphite_socket != -1)
	{
		close(graphite_socket);
		graphite_socket = -1;
	}
}

int graphite_init(const char *host, int port)
{
    if((graphite_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket"); return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port); 

    if(inet_pton(AF_INET, host, &serv_addr.sin_addr)<=0)
    {
        perror("inet_pton"); return 1;
    } 

    if( connect(graphite_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       perror("connect"); return 1;
    } 
    return 0;
}

/*******************************************************************
* functions stolen from pnp4nagios
*  https://github.com/lingej/pnp4nagios/blob/master/src/npcdmod.c
*
********************************************************************/
int grafit_process_module_args(char *args){
	char *ptr = NULL;
	char **arglist = NULL;
	char **newarglist = NULL;
	int argcount = 0;
	int memblocks = 64;
	int arg = 0;

	/* No args, no problems, using defaults */
	if (args == NULL)
		return OK;

	/* allocate some memory */
	if ((arglist = (char **) malloc(memblocks * sizeof(char **))) == NULL)
		return ERROR;

	/* process all args */
	ptr = strtok(args, ",");
	while (ptr) {
		/* save the argument */
		arglist[argcount++] = strdup(ptr);

		/* allocate more memory if needed */
		if (!(argcount % memblocks)) {
			if ((newarglist = (char **) realloc(arglist, (argcount + memblocks)
					* sizeof(char **))) == NULL) {
				for (arg = 0; arg < argcount; arg++)
					free(arglist[argcount]);
				free(arglist);
				return ERROR;
			} else
				arglist = newarglist;
		}
		ptr = strtok(NULL, ",");
	}

	/* terminate the arg list */
	arglist[argcount] = '\x0';

	/* process each argument */
	for (arg = 0; arg < argcount; arg++) {
		if (grafit_process_config_var(arglist[arg]) == ERROR) {
			for (arg = 0; arg < argcount; arg++)
				free(arglist[arg]);
			free(arglist);
			return ERROR;
		}
	}

	/* free allocated memory */
	for (arg = 0; arg < argcount; arg++)
		free(arglist[arg]);
	free(arglist);

	return OK;
	}

int grafit_process_config_file(char *filename) {
	grafit_mmapfile *thefile = NULL;
	char *buf = NULL;
	char temp_buffer[1024];
	int result = OK;

	/* open the file */
	if ((thefile = grafit_mmap_fopen(filename)) == NULL) {
		snprintf(temp_buffer, sizeof(temp_buffer) - 1,
				"grafit ERROR: failed to open %s\n", filename);
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		write_to_all_logs(temp_buffer, NSLOG_INFO_MESSAGE);
		return ERROR;

	} else {
		snprintf(temp_buffer, sizeof(temp_buffer) - 1,
				"grafit: %s initialized\n", filename);
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		write_to_all_logs(temp_buffer, NSLOG_INFO_MESSAGE);
	}

	/* process each line of the file */
	while ((buf = grafit_mmap_fgets(thefile))) {

		/* skip comments */
		if (buf[0] == '#') {
			free(buf);
			continue;
		}

		/* skip blank lines */
		if (!strcmp(buf, "")) {
			free(buf);
			continue;
		}

		/* skip new lines */
		if (!strcmp(buf, "\n")) {
			free(buf);
			continue;
		}
		/* process the variable */
		result = grafit_process_config_var(buf);

		/* free memory */
		free(buf);

		if (result != OK)
			break;
	}

	/* close the file */
	grafit_mmap_fclose(thefile);

	return result;
  }

/* process a single module config variable */
int grafit_process_config_var(char *arg) {
	char *var = NULL;
	char *val = NULL;
  char temp_buffer[1024];

	/* split var/val */
	var = strtok(arg, "=");
	val = strtok(NULL, "\n");

	/* skip incomplete var/val pairs */
	if (var == NULL || val == NULL)
		return OK;

	/* strip var/val */
	strip(var);
	strip(val);

	if (!strcmp(var, "config_file"))
		grafit_process_config_file(val);
	else if(!strcmp(var,"prefix"))
		grafit_prefix = strdup(val);
	else if (!strcmp(var, "graphite_host"))
		graphite_host = strdup(val);
	else if (!strcmp(var, "graphite_port"))
			graphite_port = (int)strtol(val,(char **)NULL, 10);
	else
		return ERROR;
	return OK;
  }

/* open a file read-only via mmap() */
grafit_mmapfile *grafit_mmap_fopen(char *filename) {
	grafit_mmapfile *new_mmapfile;
	int fd;
	void *mmap_buf;
	struct stat statbuf;
	int mode = O_RDONLY;

	/* allocate memory */
	if ((new_mmapfile = (grafit_mmapfile *) malloc(sizeof(grafit_mmapfile))) == NULL)
		return NULL;

	/* open the file */
	if ((fd = open(filename, mode)) == -1) {
		free(new_mmapfile);
		return NULL;
	}

	/* get file info */
	if ((fstat(fd, &statbuf)) == -1) {
		close(fd);
		free(new_mmapfile);
		return NULL;
	}

	/* mmap() the file */
	if ((mmap_buf = (void *) mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE,
			fd, 0)) == MAP_FAILED) {
		close(fd);
		free(new_mmapfile);
		return NULL;
	}

	/* populate struct info for later use */
	/*new_mmapfile->path=strdup(filename);*/
	new_mmapfile->path = NULL;
	new_mmapfile->fd = fd;
	new_mmapfile->file_size = (unsigned long) (statbuf.st_size);
	new_mmapfile->current_position = 0L;
	new_mmapfile->current_line = 0L;
	new_mmapfile->mmap_buf = mmap_buf;

	return new_mmapfile;
  }

/* close a file originally opened via mmap() */
int grafit_mmap_fclose(grafit_mmapfile *temp_mmapfile) {

	if (temp_mmapfile == NULL)
		return ERROR;

	/* un-mmap() the file */
	munmap(temp_mmapfile->mmap_buf, temp_mmapfile->file_size);

	/* close the file */
	close(temp_mmapfile->fd);

	/* free memory */
	if (temp_mmapfile->path != NULL)
		free(temp_mmapfile->path);
	free(temp_mmapfile);

	return OK;
}

/* gets one line of input from an mmap()'ed file */
char *grafit_mmap_fgets(grafit_mmapfile *temp_mmapfile) {
	char *buf = NULL;
	unsigned long x = 0L;
	int len = 0;

	if (temp_mmapfile == NULL)
		return NULL;

	/* we've reached the end of the file */
	if (temp_mmapfile->current_position >= temp_mmapfile->file_size)
		return NULL;

	/* find the end of the string (or buffer) */
	for (x = temp_mmapfile->current_position; x < temp_mmapfile->file_size; x++) {
		if (*((char *) (temp_mmapfile->mmap_buf) + x) == '\n') {
			x++;
			break;
		}
	}

	/* calculate length of line we just read */
	len = (int) (x - temp_mmapfile->current_position);

	/* allocate memory for the new line */
	if ((buf = (char *) malloc(len + 1)) == NULL) {
		write_to_all_logs("could not allocate a new buf", NSLOG_INFO_MESSAGE);
		return NULL;
	}

	/* copy string to newly allocated memory and terminate the string */
	memcpy(buf, ((char *) (temp_mmapfile->mmap_buf)
			+ temp_mmapfile->current_position), len);
	buf[len] = '\x0';

	/* update the current position */
	temp_mmapfile->current_position = x;

	/* increment the current line */
	temp_mmapfile->current_line++;

	return buf;
  }

/**************** end ex-pnp section ********************************/
