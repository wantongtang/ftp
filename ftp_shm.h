#ifndef _FTP_SHM_H
#define _FTP_SHM_H

int create_shm();
int get_shm_content(void *data);
int modify_shm_stat(int flg, unsigned long bytes);
int free_shm_stat();
int free_sem();

#endif	/*_FTP_SHM_H*/

