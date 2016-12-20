#define MAX_BUFF                255

#define KJAM_WIPE_CACHE			_IOW('p', 36, int)

char *filetosend;

void restart_dev(void);
void shutdown_dev(void);
void factory_reset(void);
void switch_mode(int);
void download_progress(char *, unsigned long);
int update_firmware(char **);
void findme(char* project);
char* get_hw_info(void);
void adb_mode(int);
void clear_cmd(void);

