#ifndef _FTP_CMD_H_
#define _FTP_CMD_H_

/*登陆的相关命令*/
int validate_usrname(void *data);
int validate_passwd(void *data);
int op_syst(void *data);
int op_type(void *data);

/*对文件夹的相关命令*/
int op_cwd(void *data);
int op_rmd(void *data);
int op_cdup(void *data);
int op_pwd(void *data);
int op_list(void *data);
int op_mkd(void *data);
int op_rnfr(void *data);
int op_dele(void *data);

/*文件传输的相关命令*/
int op_pasv(void *data);
int op_port(void *data);
int op_rest(void *data);
int op_appe(void *data);
int op_rnto(void *data);
int op_stor(void *data);
int op_retr(void *data);

/*其他命令*/
int op_noop(void *data);
int op_help(void *data);
int op_stat(void *data);
int op_quit(void *data);
int op_abor(void *data);
int op_rein(void *data);
int op_size(void *data);
int op_mode(void *data);

/*信号*/
void sig_abor(int signum);

#endif /*_FTP_CMD_H_*/
