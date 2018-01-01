#ifndef _FTP_SOCK_H_
#define _FTP_SOCK_H_

#define MAXLISTEN 5
#define TIMEWAIT  120 


void create_daemon();
int u_open(short port);
int r_accept(int sock_fd);
int r_connect(int sock_fd, void *cliaddr);
int select_timeout(int sock_fd);
int create_pasv(int sock_fd);
int create_post();

void get_pidof(char *pro_buf, int buf_len, const char *argv);
int check_process(char *pro_buf);
void myftp_start(char *pro_buf);
void myftp_stop(char *pro_buf);
void mgr_myftp(int argc, char *argv[]);

int r_read(int sock_fd, char *buf, int len);
int r_write(int sock_fd, char *buf, int len);

#endif /*_FTP_SOCK_H_*/
