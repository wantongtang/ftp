#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "ftp_define.h"
#include "ftp_shm.h"
#include "ftp_sock.h"
#include "ftp_cmd.h"

long upload;
long download;
struct timeval time_val;
struct timeval *time_val_ptr;

int main(int argc, char *argv[])
{
	int local_socket;
	int accept_socket;
	pid_t child;
	int read_count = 0;
	char buf[512] = {0};
	int retval = 0;
	char remsg[128] = {0};
	char key[128] = {0};
	char data[128] = {0};
	char conf_val[3][40] = {0};
	int port;
	
	mgr_myftp(argc, argv);
	read_config(conf_val);		//读取配置文件信息
	port = atoi(conf_val[0]);
	upload = atoi(conf_val[1]);
	download = atoi(conf_val[2]);
	create_daemon();			//创建守护进程
	
	signal(SIGCHLD, SIG_IGN);
	local_socket = u_open(port);
	if (local_socket == -1)
	{
		return 0;
	}
	
	create_shm();

	while (1)
	{
		accept_socket = r_accept(local_socket);
		if (accept_socket == -1)
		{
			break;
		}

		CMDINFO info_cmd;
		info_cmd.acc_socket = accept_socket;
		strcpy(remsg, "220 (zhrftp v1.0)\r\n");
		r_write(accept_socket, remsg, strlen(remsg));	
		info_cmd.rein_flg = 0;
		
		child = fork();
		if (child == 0)
		{
			close(local_socket);
			time_val.tv_sec = TIMEWAIT;
//			printf("time_val = %d\n", time_val.tv_sec);
			time_val_ptr = &time_val;
			while (1)
			{	
				if (time_val_ptr != NULL)
				{
					time_val.tv_sec = TIMEWAIT;
//					printf("time_val = %d\n", time_val.tv_sec);
					time_val_ptr = &time_val;
				}
				retval = select_timeout(accept_socket);
				if (retval > 0)
				{
					read_count = r_read(accept_socket, buf, sizeof(buf));
					if (read_count > 0)
					{
						parse_cmd(buf, key, data);			//命令解析
						strcpy(info_cmd.data, data);
						deal_cmd(key, (void *)&info_cmd);	//根据命令需找相应的处理函数
						memset(buf, 0x00, sizeof(buf));
						memset(info_cmd.data, 0x00, sizeof(info_cmd.data));
					}
					else if (read_count == 0)
					{
						break;
					}

				}
				else if (retval == 0)
				{
					printf("select timeout!\n");
					break;
				}
			}
			close(accept_socket);
			exit(0);
		}
		else if(child > 0)
		{
			close(accept_socket);		
		}	
	}
	return 0;
}
