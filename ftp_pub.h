#ifndef _FTP_PUB_
#define _FTP_PUB_

/*解析命令的相关操作*/
void parse_cmd(char *buf, char *key, char *data);
void deal_cmd(char *key, void *data);
void upper_string(char *str);
void lower_string(char *str);
void parse_cliaddr(char *buf, void *cliaddr);
int change_popedom(char *usrname);
int is_permission(char *usrname);

/*文件的相关操作*/
void recv_file(int sock_fd, char *filename, int flg, int pos);
void send_file(int sock_fd, char *filename, int pos);
int read_config(char values[][40]);

#endif	/*_FTP_PUB_*/
