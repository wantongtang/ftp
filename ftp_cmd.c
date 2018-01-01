#define _XOPEN_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <shadow.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pwd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "ftp_define.h"
#include "ftp_shm.h"
#include "ftp_cmd.h"
#include "ftp_sock.h"
#include "ftp_myls.h"
#include "ftp_pub.h"

int g_trans_flg = 0;
extern struct timeval time_val;
extern struct timeval *time_val_ptr;

OPCMD opcmd_array[] = {
	{"USER", validate_usrname},
	{"PASS", validate_passwd},
	{"SYST", op_syst},
	{"REST", op_rest},
	{"PWD", op_pwd}, {"XPWD", op_pwd},
	{"CWD", op_cwd}, {"XCWD", op_cwd},
	{"CDUP", op_cdup},
	{"TYPE", op_type},
	{"PASV", op_pasv},
	{"PORT", op_port},
	{"LIST", op_list},
	{"NOOP", op_noop},
	{"HELP", op_help},
	{"QUIT", op_quit},
	{"ABOR", op_abor},
	{"MKD", op_mkd}, {"XMKD", op_mkd},
	{"RMD", op_rmd}, {"XRMD", op_rmd},
	{"RNFR", op_rnfr},
	{"RNTO", op_rnto},
	{"STOR", op_stor},
	{"RETR", op_retr},
	{"DELE", op_dele},
	{"STAT", op_stat},
	{"APPE", op_appe},
	{"QUIT", op_quit},
	{"REIN", op_rein},
	{"SIZE", op_size},
	{"MODE", op_mode},
	{NULL, NULL}
};


/*处理用户名命令*/
int validate_usrname(void *data)
{
	char remsg[64] = "331 Please specify the password.\r\n";
	CMDINFO *info_cmd = (CMDINFO *)data;

	strcpy(info_cmd->usrname, info_cmd->data);	
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));

	return 1;	
}

/*处理密码命令*/
int validate_passwd(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char code_key[13] = {0};
	char usrname[32] = {0};
	char remsg[64] = "230 Login successful. Have fun.\r\n";

	struct spwd *spwd_ptr;

	strcpy(usrname, info_cmd->usrname);
	upper_string(usrname);
	if (strcmp(usrname, "ANONYMOUS") == 0 )
	{
//		char  path[128] = {0};
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));
		strcpy(info_cmd->usrname, "ftp");
		change_popedom(info_cmd->usrname);
	
//		getcwd(path, sizeof(path));
//		change_popedom("root");
//		chdir(path);

		info_cmd->rein_flg = 0;
		return 1;
	}

	spwd_ptr = getspnam(info_cmd->usrname);

	if (spwd_ptr == NULL)
	{
		strcpy(remsg, "530 Login incorrect.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
		return 0;
	}

	strncpy(code_key, spwd_ptr->sp_pwdp, 12);
	if (strcmp(spwd_ptr->sp_pwdp, crypt(info_cmd->data, code_key)) == 0)
	{
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));
		change_popedom(info_cmd->usrname);
		info_cmd->rein_flg = 0;
		return 1;
	}
	else 
	{
		strcpy(remsg, "530 Login incorrect.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
		return 0;
	}
}

/*处理SYST命令*/
int op_syst(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "215 UNIX Type: L8\r\n";

	r_write(info_cmd->acc_socket, remsg, strlen(remsg));

	return 1;
}

/*处理REST命令*/
int op_rest(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = {0};

	sprintf(remsg, "350 Restart position accepted (%d)\r\n", atoi(info_cmd->data));
	strcpy(info_cmd->buf, info_cmd->data);
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));

	return 1;
}

/*处理PWD命令*/
int op_pwd(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char path[256] = {0};
	char remsg[512] = {0};

	getcwd(path, sizeof(path));
	if (strcmp(info_cmd->usrname, "ftp") == 0)
	{
		strcpy(path+1, path+9);
	}
	sprintf(remsg, "257 \"%s\" is current directory.\r\n", path);
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));

	return 1;
}

/*处理CWD命令*/
int op_cwd(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[256] = {0};
	
	chdir(info_cmd->data); 
	sprintf(remsg, "257 Directory change to \"%s\"\r\n", info_cmd->data);
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));

	return 1;
}

/*处理CDUP命令*/
int op_cdup(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char path[256] = {0};
	char remsg[512] = {0};
	
	chdir(".."); 
	getcwd(path, sizeof(path));
	sprintf(remsg, "257 \"%s\"\r\n", path);
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));

	return 1;
}

/*处理TYPE命令*/
int op_type(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "200 Switching to ASCII mode.\r\n";

	if (strcmp(info_cmd->data, "I") == 0)
	{
		strcpy(remsg, "200 Switching to Binary mode.\r\n");
	}

	r_write(info_cmd->acc_socket, remsg, strlen(remsg));

	return 1;
}

/*处理PASV命令*/
int op_pasv(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[128] = {0};
	int pasv_socket;
	struct sockaddr_in pasv_addr;
	int pasv_len = sizeof(pasv_addr);
	int port;
	char ip_addr[16] = {0};
	int  i = 0;
	int fd;

	if ((pasv_socket = create_pasv(info_cmd->acc_socket)) == -1)
	{
		return 0;
	}

	if (getsockname(pasv_socket, (struct sockaddr*)&pasv_addr, &pasv_len) == -1)
	{
		perror("get pasv_socket name error:");
		return 0;
	}

	strcpy(ip_addr, inet_ntoa(pasv_addr.sin_addr));
	while (ip_addr[i] != '\0')
	{
		if (ip_addr[i] == '.')
		{
			ip_addr[i] = ',';
		}
		i++;
	}

	port = ntohs(pasv_addr.sin_port);
	sprintf(remsg, "227 Entering Passive Mode (%s,%d,%d)\r\n", ip_addr, port/256, port%256);
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));

	fd = r_accept(pasv_socket);
	info_cmd->data_socket = fd;
	return 1;
}

/*处理PORT命令*/
int op_port(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	struct sockaddr_in cli_addr;
	int port_socket;
	char path[256] = {0};

	parse_cliaddr(info_cmd->data, (void *)&cli_addr);

	getcwd(path, sizeof(path));
	change_popedom("root");
	
	if ((port_socket = create_port(info_cmd->acc_socket)) == -1)
	{
		return 0;
	}	

	change_popedom(info_cmd->usrname);
	if (chdir(path) != 0)
	{
		return 0;
	}
 
	r_connect(port_socket ,&cli_addr);
	info_cmd->data_socket = port_socket;

	char remsg[64] = "200 PORT command successful. Consider using PASV.\r\n";
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));
	return 1;
}

/*将目录发送给客户端*/
int op_list(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "150 Here comes the directory listing.\r\n";
	pid_t child;
	
	signal(SIGCHLD, SIG_IGN);
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));
	
	child = fork();
	
	if (child == 0)
	{
		get_myls(info_cmd->data_socket);
		close(info_cmd->data_socket);

		strcpy(remsg, "226 Directory send OK.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));
		exit(0);
	}
	else if (child > 0)
	{
		close(info_cmd->data_socket);
		return 1;
	}
	else
	{
		return 0;
	}
}

/*处理NOOP命令*/
int op_noop(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "200 NOOP ok.\r\n";
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));	

	return 1;
}

/*处理HELP命令*/
int op_help(void *data)
{
	int i = 0;
	CMDINFO *info_cmd = (CMDINFO *)data;
	char *remsg[64] = {"214 The following commands are recognized (* => unimplemented).\r\n",\
						"USER   PASS   CWD    XCWD   CDUP   REIN\r\n",\
						"QUIT   PORT   PASV   TYPE   MODE   RETR\r\n",\
						"STOR   APPE   REST   RNFR   ABOR   DELE\r\n",\
						"RMD    XRMD   MKD    XMKD   PWD    XPWD\r\n",\
						"LIST   SYST   STAT   SIZE   HELP   NOOP\r\n",\
						"214 Direct comments or bugs to zhr@zhr.com.\r\n", "NULL"};	
		
	while (remsg[i] != "NULL")
	{
		r_write(info_cmd->acc_socket, remsg[i], strlen(remsg[i]));
		i++;
	}

	return 1;
}

/*处理MKD命令*/
int op_mkd(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[256] = {0};
	char path[128] = {0};
	
	if (is_permission(info_cmd->usrname) == 0)
	{
		strcpy(remsg, "550 Permission denied.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
		return 0;
	}

	getcwd(path, sizeof(path));
	if (mkdir(info_cmd->data, 0755) != 0)
	{
		strcpy(remsg, "550 Create directory operation failed.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));
		perror("mkdir error:");
		return 0; 
	}
	sprintf(remsg, "257 \"%s/%s\" created.\r\n", path, info_cmd->data);
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));	

	return 1;
}

/*处理RMD命令*/
int op_rmd(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "250 Remove directory operation successful.\r\n";

	if (is_permission(info_cmd->usrname) == 0)
	{
		strcpy(remsg, "550 Permission denied.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
		return 0;
	}

	if (rmdir(info_cmd->data) != 0)
	{
		perror("rmdir error:");
		return 0;
	}

	r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
	return 1;
}

/*处理RNFR命令*/
int op_rnfr(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "350 Ready for RNTO.\r\n";

	if (is_permission(info_cmd->usrname) == 0)
	{
		strcpy(remsg, "550 Permission denied.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
		return 0;
	}

	strcpy(info_cmd->buf, info_cmd->data);
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
	
	return 1;
}

/*处理RNTO命令*/
int op_rnto(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "250 Rename successful.\r\n";

	if (rename(info_cmd->buf, info_cmd->data) != 0)
	{
		return 0;
	}

	memset(info_cmd->buf, 0x00, sizeof(info_cmd->buf));
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));
	return 1;
}

/*处理STOR命令*/
int op_stor(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "150 Ok to send data.\r\n";
	pid_t child;
	struct stat stat_buf;
	int pos = 0;
	signal(SIGCHLD, SIG_IGN);
	signal(35, sig_abor);
	
	if (is_permission(info_cmd->usrname) == 0)
	{
		strcpy(remsg, "550 Permission denied.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
		return 0;
	}

	g_trans_flg = 1;
	time_val_ptr = NULL;

	child = fork();
	if (child == 0)
	{
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));
		pos = atoi(info_cmd->buf);
		
		recv_file(info_cmd->data_socket, info_cmd->data, STOR, pos);

		strcpy(remsg, "226 File receive OK.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));
			
//		usleep(500000);
//		sleep(1);
		kill(getppid(), 35);
		
		lstat(info_cmd->data, &stat_buf);

		if (modify_shm_stat(0, stat_buf.st_size) != 1)
		{
			return 0;
		}
		exit(0);
	}
	else if (child > 0)
	{
		memset(info_cmd->buf, 0x00, sizeof(info_cmd->buf));
		close(info_cmd->data_socket);
		return 1;
	}
	else
	{
		return 0;
	}
}

/*处理RETR命令*/
int op_retr(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[128] = {0};
	pid_t child;
	struct stat stat_buf;	
	int pos = 0;
	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(35, sig_abor);
	
	g_trans_flg = 1;
	time_val_ptr = NULL;

	child = fork();
	if (child == 0)
	{
		lstat(info_cmd->data, &stat_buf);

		sprintf(remsg, "150 Opening BINARY mode data connection for %s. (%ld bytes)\r\n",\
			info_cmd->data, stat_buf.st_size);
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));

		pos = atoi(info_cmd->buf);
		send_file(info_cmd->data_socket, info_cmd->data, pos);
		
		strcpy(remsg, "226 File send OK.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));

//		usleep(500000);
//		sleep(1);
		kill(getppid(), 35);

		if (modify_shm_stat(1, stat_buf.st_size) != 1)
		{
			return 0;
		}
		exit(0);
	}
	else if (child >0)
	{
		memset(info_cmd->buf, 0x00, sizeof(info_cmd->buf));
		close(info_cmd->data_socket);
		return 1;
	}
	else
	{
		return 0;
	}	
}

/*处理DELE命令*/
int op_dele(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "250 Delete operation successful.\r\n";
	
	if (is_permission(info_cmd->usrname) == 0)
	{
		strcpy(remsg, "550 Permission denied.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
		return 0;
	}

	if (unlink(info_cmd->data) != 0)
	{
		strcpy(remsg, "550 Delete operation failed.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
		perror("rmdir error:");
		return 0;
	}

	r_write(info_cmd->acc_socket, remsg, strlen(remsg));		
	return 1;
}

/*处理STAT命令*/
int op_stat(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	SHSTAT stat_shm;
	char remsg[128] = {0};
	
	if (strcmp(info_cmd->usrname, "ftp") == 0)
	{
		sprintf(remsg, "211 Status for user anonymous.\r\n", info_cmd->usrname);
	}
	else
	{
		sprintf(remsg, "211 Status for user %s.\r\n", info_cmd->usrname);
	}
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
	get_shm_content(&stat_shm);

/*
	printf("stor_count = %d\n", stat_shm.stor_count);
	printf("stor_bytes = %d\n", stat_shm.stor_bytes);
	printf("retr_count = %d\n", stat_shm.retr_count);
	printf("retr_count = %d\n", stat_shm.retr_bytes);
*/
	sprintf(remsg, "    Stored %d files, %d kBytes\r\n", stat_shm.stor_count, stat_shm.stor_bytes);
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));	

	sprintf(remsg, "    Retrieved %d files, %d kBytes\r\n", stat_shm.retr_count, stat_shm.retr_bytes);
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));	

	strcpy(remsg, "211 End of status.\r\n");
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
	return 1;
}

/*处理APPE命令*/
int op_appe(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "150 Ok to send data.\r\n";
	pid_t child;
	struct stat stat_buf;
	signal(SIGCHLD, SIG_IGN);
	signal(35, sig_abor);
	
	g_trans_flg = 1;
	time_val_ptr = NULL;

	child = fork();
	if (child == 0)
	{
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));
		recv_file(info_cmd->data_socket, info_cmd->data, APPE, 0);

		strcpy(remsg, "226 File receive OK.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));	

//		usleep(500000);
//		sleep(1);
		kill(getppid(), 35);

		lstat(info_cmd->data, &stat_buf);

		if (modify_shm_stat(0, stat_buf.st_size) != 1)
		{
			return 0;
		}
		exit(0);
	}
	else if (child > 0)
	{
		close(info_cmd->data_socket);
		return 1;
	}
	else
	{
		return 0;
	}
}

/*处理QUIT命令*/
int op_quit(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "221 GoodBye.\r\n";
	
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
	close(info_cmd->acc_socket);

	return 1;
}

/*处理ABOR命令*/
int op_abor(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "225 No trnansfer to ABOR.\r\n";
		
	close(info_cmd->data_socket);	
	while (1)
	{
		if (g_trans_flg == 0)
		{
			r_write(info_cmd->acc_socket, remsg, strlen(remsg));
			break;
		}
		else 
		{
			continue;
		}
	}

/*
	if (g_trans_flg == 0)
	{
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));
	}

	else 
	{
		strcpy(remsg, "226 Abor success.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));
	}
*/
	return 1;
}

/*处理REIN命令*/
int op_rein(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = "220 Service ready for new user.\r\n";
	
	info_cmd->rein_flg = 1;
	memset(info_cmd->usrname, 0x00, sizeof(info_cmd->usrname));
	memset(info_cmd->data, 0x00, sizeof(info_cmd->data));
	memset(info_cmd->buf, 0x00, sizeof(info_cmd->buf));
	change_popedom("root");

	r_write(info_cmd->acc_socket, remsg, strlen(remsg));	
	return 1;
}

/*处理SIZE命令*/
int op_size(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = {0};
	char temp[128] = {0};
	struct stat stat_buf;
	
	if (strcmp(info_cmd->data, "\0") == 0)
	{
		strcpy(remsg, "501 Syntax error in parameters or arguments.\r\n");
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));
		return 0;
	}
	
	strcpy(temp, info_cmd->data);
	lower_string(temp);
	if (access(temp, F_OK) != 0)
	{
		sprintf(remsg, "550 %s: No such file.\r\n", info_cmd->data);
		r_write(info_cmd->acc_socket, remsg, strlen(remsg));
		return 0;
	}

	lstat(temp, &stat_buf);
	sprintf(remsg, "213 %d\r\n", stat_buf.st_size);
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));

	return 1;
}

/*处理MODE命令*/
int op_mode(void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	char remsg[64] = {0};
	
	upper_string(info_cmd->data);
	if (strcmp(info_cmd->data, "S") == 0 || strcmp(info_cmd->data, "Z") == 0)
	{
		sprintf(remsg, "200 MODE %s ok.\r\n", info_cmd->data);
	}
	else
	{
		strcpy(remsg, "504 Command not implemented for that parameter.\r\n");
	}
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));

	return 1;
}

/*调用命令相对应的处理函数*/
void deal_cmd(char *key, void *data)
{
	CMDINFO *info_cmd = (CMDINFO *)data;
	int i = 0;
	char remsg[64] = "500 Unknown command.\r\n";

	if (info_cmd->rein_flg == 1)
	{
		if (strcmp(info_cmd->usrname, "\0") != 0)
		{
			if (strcmp(key, "PASS") != 0)
			{
				strcpy(remsg, "530 Not logged in.\r\n");
				r_write(info_cmd->acc_socket, remsg, strlen(remsg));
				return;
			}
		}
		else 
		{
			if (strcmp(key, "PASS") == 0)
			{
				strcpy(remsg, "503 Bad sequence of commands.\r\n");
				r_write(info_cmd->acc_socket, remsg, strlen(remsg));
				return;
			}

			if(strcmp(key, "USER") != 0)
			{
				strcpy(remsg, "530 Not logged in.\r\n");
				r_write(info_cmd->acc_socket, remsg, strlen(remsg));
				return;
			}
		}
	}

//	printf("key = %s\n", key);
	while (opcmd_array[i].cmd_key != NULL)
	{
		if (strcmp(key, opcmd_array[i].cmd_key) == 0)
		{
			opcmd_array[i].opfun(data);
			return;
		}
		i++;
	}
	r_write(info_cmd->acc_socket, remsg, strlen(remsg));
}

/*传输结束信号*/
void sig_abor(int signum)
{
	g_trans_flg = 0;
//	printf("time_val = %d\n", time_val.tv_sec);
	time_val.tv_sec = TIMEWAIT;
	time_val_ptr = &time_val;
//	printf("time_val = %d\n", time_val.tv_sec);
}
