#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>

#include "ftp_sock.h"
#include "ftp_shm.h"

//extern struct timeval time_val;
extern struct timeval *time_val_ptr;

/*创建守护进程*/
void create_daemon()
{
	pid_t child;
	
	child = fork();
	if (child == 0)
	{
		signal(SIGHUP, SIG_IGN);
		setsid();
		chdir("/");
		child = fork();
		if (child == 0)
		{
			
		}
		else if (child > 0)
		{
			exit(1);
		}
	}
	else if (child > 0)
	{
		exit(0);
	}
}

/*创建服务器端套接字*/
int u_open(short port)
{
	int flg = 1;
	struct sockaddr_in srv_addr;
	int local_socket;
	
	if ((local_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("create server socket error:");
		return -1;
	}

	if (setsockopt(local_socket, SOL_SOCKET, SO_REUSEADDR, &flg, sizeof(flg)) == -1)
	{
		perror("setcockopt error:");
		return -1;
	}

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	srv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(local_socket, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) == -1 || listen(local_socket, MAXLISTEN) == -1)
	{
		perror("bind socket error:");
		return -1;
	}
	
	return local_socket;
}

/*创建PASV套接字*/
int create_pasv(int sock_fd)
{
	struct sockaddr_in pasv_addr;
	int pasv_socket;
	int pasv_len = sizeof(pasv_addr);

	if ((pasv_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("create pasv socket error:");
		return -1;
	}

	if (getsockname(sock_fd, (struct sockaddr*)&pasv_addr, &pasv_len) == -1)
	{
		perror("get accept_socket name error:");
		return -1;
	}
	
	pasv_addr.sin_port = htons(0);
	
	if (bind(pasv_socket, (struct sockaddr*)&pasv_addr, pasv_len) == -1 || listen(pasv_socket, MAXLISTEN) == -1)
	{
		perror("bind pasv socket error:");
		return -1;
	}

	return pasv_socket;
}

/*创建PORT套接字*/
int create_port(int sock_fd)
{
	int port_socket;
	struct sockaddr_in srv_addr;
	int srv_len = sizeof(srv_addr);
	int flg = 1;

	if ((port_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("create port socket error:");
		return -1;
	}

	if (setsockopt(port_socket, SOL_SOCKET, SO_REUSEADDR, &flg, sizeof(flg)) == -1)
	{
		perror("setcockopt error:");
		return -1;
	}

	if (getsockname(sock_fd, (struct sockaddr*)&srv_addr, &srv_len) == -1)
	{
		perror("get accept_socket name error:");
		return -1;
	}

	srv_addr.sin_port = htons(20);

	if (bind(port_socket, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) == -1)
	{
		perror("bind port socket error:");
		return -1;
	}

	return port_socket;
}

/*等待客户端连接*/
int r_accept(int sock_fd)
{
	struct sockaddr_in cli_addr;
	int len = sizeof(struct sockaddr);
	int retval = 0;

	while ((retval = accept(sock_fd, (struct sockaddr*)&cli_addr, &len)) == -1 && errno == EINTR);
//	{
//		perror("accept error:");
//	}

	return retval;
}

/*连接客户端*/
int r_connect(int sock_fd, void *cliaddr)
{
	int retval;
	struct sockaddr_in *cli_addr = (struct sockaddr_in *)cliaddr;
	
	cli_addr->sin_family = AF_INET;

	while (retval = connect(sock_fd, (struct sockaddr*)cli_addr, sizeof(struct sockaddr_in)) == -1 && errno == EINTR);
	
	return retval;
}

/*超时处理*/
int select_timeout(int sock_fd)
{
	int retval;
	fd_set sock_set;
//	struct timeval time_val;
//	time_val.tv_sec = TIMEWAIT;

	FD_ZERO(&sock_set);
	FD_SET(sock_fd, &sock_set);

//	printf("time_val = %d\n", time_val_ptr->tv_sec);
//	while ((retval = select(sock_fd+1, &sock_set, NULL, NULL, &time_val)) == -1 && errno == EINTR);
//	while ((retval = select(sock_fd+1, &sock_set, NULL, NULL, NULL)) == -1 && errno == EINTR);
	while ((retval = select(sock_fd+1, &sock_set, NULL, NULL, time_val_ptr)) == -1 && errno == EINTR);
//	{
//		perror("select error:");
//	}
	return retval;
}

/*获取相关进程的PID*/
void get_pidof(char *pro_buf, int buf_len, const char *argv)
{
	pid_t child;
	int fd[2];
	pipe(fd);
	signal(SIGCHLD, SIG_IGN);
	child = fork();
	if (child == 0)
	{
		close(fd[0]);
		dup2(fd[1], 1);
		close(fd[1]);
		execlp("pidof", "pidof", argv, NULL);
		exit(0);
	}
	else if (child > 0)
	{
		close(fd[1]);
		read(fd[0], pro_buf, buf_len);
	}
}

/*判断服务器进程是否唯一*/
int check_process(char *pro_buf)
{
	int i = 0;
	int count = 0;
//	char *tmp_buf = pro_buf;

	while (pro_buf[i] != '\0')
	{
		if (pro_buf[i] == ' ')
		{
			count++;
		}
		i++;
	}	
	pro_buf[(i-1)] = '\0';
	return count;
}

/*启动服务器*/
void myftp_start(char *pro_buf)
{
	int count = 0;
	int i = 0;
	count = check_process(pro_buf);
	if (count > 0)
	{
		printf("myftp is running......\n");	
		exit(0);
	}
	printf("myftp start ");

	for (i=0; i<35; i++)
	{	
		fflush(stdout);
		printf(".");
		usleep(10000);
	}
	printf("[OK]\n");
}

/*关闭服务器*/
void myftp_stop(char *pro_buf)
{
	int i = 0;
	char buf[16] = {0};
	int pos = 0;
	int kill_id;
	int count = 0;

	count = check_process(pro_buf);
	if (count == 0)
	{
		printf("myftp is not running......\n");	
		return;
	}

	while (1)
	{
		if (pro_buf[i] == ' ' || pro_buf[i] == '\0')
		{
			buf[++pos] = '\0';	
			kill_id = atoi(buf);
//			printf("curp_id = %d\n", getpid());
//			printf("kill_id = %d\n", kill_id);
			if (kill_id != getpid() && kill_id != 0)
			{
				kill(kill_id, SIGKILL);
			}

			if (pro_buf[i] == '\0')
			{	
				break;
			}
			memset(buf, 0x00, sizeof(buf));
			pos = 0;			
			i++;
			continue;
		}
		pos += sprintf(buf+pos, "%c", pro_buf[i]);		
		i++;
	}

	free_shm_stat();	//释放共享内存
	free_sem();			//释放信号量

	printf("myftp stop  ");

	for (i=0; i<35; i++)
	{	
		fflush(stdout);
		printf(".");
		usleep(10000);
	}
	printf("[OK]\n");
}

/*服务器的管理*/
void mgr_myftp(int argc, char *argv[])
{
	char pro_buf[512] = {0};
	int count = 0;

	if (argc == 1 || argc > 2)
	{
		printf("usecmd:./myftp <start> <stop> <restart>\n");
		exit(1);
	}

	get_pidof(pro_buf, sizeof(pro_buf), argv[0]);
	if (strcmp(argv[1], "start") == 0)
	{
		myftp_start(pro_buf);
	}
	else if (strcmp(argv[1], "restart") == 0)
	{
		myftp_stop(pro_buf);
		sleep(1);
		memset(pro_buf, 0x00, sizeof(pro_buf));
		get_pidof(pro_buf, sizeof(pro_buf), argv[0]);
		myftp_start(pro_buf);
//		printf("myftp restart!\n");
	}
	else if (strcmp(argv[1], "stop") == 0)
	{
		myftp_stop(pro_buf);
		exit(1);
	}
	else
	{
		printf("usecmd:./myftp <start> <stop> <restart>\n");
		exit(1);
	}
}

/*读取套接字内容*/
int r_read(int sock_fd, char *buf, int len)
{
	int read_count;
	while((read_count = read(sock_fd, buf, len)) == -1 && errno == EINTR);
	return read_count;
}

/*写入套接字内容*/
int r_write(int sock_fd, char *buf, int len)
{
	int write_count;
	while ((write_count = write(sock_fd, buf, len)) == -1 && errno == EINTR);
	return write_count;
}
