/*********************************************************
*
* grafit.h - header file for grafit NEB module
*
*	ehl0 (Laszlo Kollar)
*    http://kollarlaszlo.hu
*    daddy@kollarlaszlo.hu)
*
*
***********************************************************/


/* stolen from grafit4nagios */
typedef struct grafit_mmapfile_struct {
        char *path;
        int mode;
        int fd;
        unsigned long file_size;
        unsigned long current_position;
        unsigned long current_line;
        void *mmap_buf;
} grafit_mmapfile;


grafit_mmapfile *grafit_mmap_fopen(char *); /* open a file read-only via mmap() */
int grafit_mmap_fclose(grafit_mmapfile *);
char *grafit_mmap_fgets(grafit_mmapfile *);

/*********************************************/

#define HOST 9
#define SERVICE 11

int grafit_handle_data(int, void *);
int grafit_process_module_args(char *);
int grafit_process_config_file(char *);
int grafit_process_config_var(char *);
int send_perfdata(char *);
int free_perfdata_str(char *);
int graphite_init(const char *host, int port);
void graphite_disconnect();
