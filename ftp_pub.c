#include <stdio.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <sys/time.h>

#include "ftp_define.h"
#include "ftp_pub.h"

extern long upload;
extern long download;

/*更改用户权限*/
int change_popedom(char *usrname)
{
	struct passwd *pwd_ptr;

	pwd_ptr = getpwnam(usrname);

	if (pwd_ptr != NULL)
	{
		if (setegid(pwd_ptr->pw_gid) != 0)
		{
			return 0;
		}
		if (seteuid(pwd_ptr->pw_uid) != 0)
		{
			return 0;
		}	
		if (chdir(pwd_ptr->pw_dir) != 0)
		{
			return 0;
		}
//		printf("change popedom success!\n");
		return 1;
	}

	return 0;
}

/*解析FTP命令*/
void parse_cmd(char *buf, char *key, char *data)
{
	int i = 0;
	int space_pos = 0;
	char tmp[64] = {0};

	while(1)
	{
		if (buf[i] == ' ')
		{
			strncpy(key, buf, i);
			key[i] = '\0';
			upper_string(key);
			space_pos = i;
			i++;
		}
		else if (buf[i] == '\r')
		{
			if (space_pos == 0)
			{
				strcpy(key, buf);
				key[i] = '\0';
				upper_string(key);
				data[0] = '\0';
			}
			else
			{
				strncpy(data, buf+space_pos+1, i-space_pos-1);
				data[(i-space_pos-1)] = '\0';
			}
			break;
		}
		else 
		{
			i++;
		}		
	}
}

/*解析客户端的地址*/
void parse_cliaddr(char *buf, void *cliaddr)
{
	int i =	0;
	int count = 0;
	int pos = 0;
	struct sockaddr_in *cli_addr = (struct sockaddr_in *)cliaddr;
	char ipaddr[20] = {0};
	char mod[8] = {0};
	char dev[8] = {0};
	int port = 0;

	while (1)
	{
		if (buf[i] == ',')
		{
			buf[i] = '.';
			count++;
			if (count == 4)
			{	
				strncpy(ipaddr, buf, i);
				ipaddr[(i+1)] = '\0';
				cli_addr->sin_addr.s_addr = inet_addr(ipaddr);
//				printf("ipaddr = [%s]\n", inet_ntoa(cli_addr->sin_addr));
				pos = i+1;
			}
			if (count == 5)
			{
				strncpy(dev, buf+pos, i-pos);
				dev[(i-pos+1)] = '\0';
				port = atoi(dev) * 256;
//				printf("dev = [%s]\n", dev);
				pos = i+1;
			}
		}
		else if (buf[i] == '\0')
		{
			strncpy(mod, buf+pos, i-pos);
			mod[(i-pos+1)] = '\0';
			port += atoi(mod);
			cli_addr->sin_port = htons(port);
//			cli_addr->sin_port = port;
//			printf("mod = [%s]\n", mod);
//			printf("port = %d\n", cli_addr->sin_port);
			break;
		}
		i++;
	}
}

/*将字符串转换成大写*/
void upper_string(char *str)
{
	int i = 0;
	while (str[i] != '\0')
	{
		str[i] = toupper(str[i]);
		i++;
	}
}

/*将字符串转换成小写*/
void lower_string(char *str)
{
	int i = 0;
	while (str[i] != '\0')
	{
		str[i] = tolower(str[i]);
		i++;
	}
}

/*判断是否为匿名用户权限*/
int is_permission(char *usrname)
{
	if (strcmp(usrname, "ftp") == 0)
	{
		return 0;
	}
	return 1;
}

/*接收文件*/
void recv_file(int sock_fd, char *filename, int flg, int pos)
{
	FILE *file = NULL;
	char buf[1024] = {0};
	int read_count = 0;
	int per_sec_count = upload*1024;
	struct timeval tm_new;
	struct timeval tm_old;
	struct timezone tm_zone;

	if (flg == STOR)
	{
		if (pos == 0)
		{
			file = fopen(filename, "wb");
		}
		else 
		{
			file = fopen(filename, "ab+");
		}
	}
	else if (flg == APPE)
	{
		file = fopen(filename, "ab+");
	}

	if (gettimeofday(&tm_old, &tm_zone) == -1)
	{
		perror("gettimeofday error:");
		return;
	}

	while((read_count = read(sock_fd, buf, sizeof(buf))) != 0)
	{
		fwrite(buf, sizeof(char), read_count, file);
		memset(buf, 0x00, sizeof(buf));

		if (upload == 0)
		{
			continue;
		}

		per_sec_count -= read_count;
		if (per_sec_count <= 0)
		{
			long dif_usec = 0;
			per_sec_count = upload*1024;
			if (gettimeofday(&tm_new, &tm_zone) == -1)
			{
				perror("gettimeofday error:");
				return;
			}	
			dif_usec = abs(tm_new.tv_sec*1000000 + tm_new.tv_usec - tm_old.tv_sec*1000000 - tm_old.tv_usec);
			if (dif_usec < 1000000)
			{
				usleep((1000000 - dif_usec));
			}
			if (gettimeofday(&tm_old, &tm_zone) == -1)
			{
				perror("gettimeofday error:");
				return;
			}
		}
	}
	fclose(file);
	close(sock_fd);
}

/*发送文件*/
void send_file(int sock_fd, char *filename, int pos)
{
	FILE *file = NULL;
	char buf[1024] = {0};
	int read_count = 0;
	int per_sec_count = download*1024;
	struct timeval tm_new;
	struct timeval tm_old;
	struct timezone tm_zone;

	file = fopen(filename, "rb");
	if (fseek(file, pos, SEEK_SET) < 0)
	{
		perror("fseek error:");
		return;
	}
	while((read_count = fread(buf, sizeof(char), sizeof(buf), file)) != 0)
	{
		r_write(sock_fd, buf, read_count);
		memset(buf, 0x00, sizeof(buf));
		
		if (download == 0)
		{
			continue;
		}

		per_sec_count -= read_count;
		if (per_sec_count <= 0)
		{
			long dif_usec = 0;
			per_sec_count = download*1024;
			if (gettimeofday(&tm_new, &tm_zone) == -1)
			{
				perror("gettimeofday error:");
				return;
			}	
			dif_usec = abs(tm_new.tv_sec*1000000 + tm_new.tv_usec - tm_old.tv_sec*1000000 - tm_old.tv_usec);
			if (dif_usec < 1000000)
			{
				usleep((1000000 - dif_usec));
			}
			if (gettimeofday(&tm_old, &tm_zone) == -1)
			{
				perror("gettimeofday error:");
				return;
			}
		}
	}
	fclose(file);
	close(sock_fd);
}

/*读取配置文件信息*/
int read_config(char values[][40])
{
	int i,pos;
	char *p;
	char objects[][30]={"port", "upload", "download"};
	char config_name[]="ftp.conf";
	FILE *fp=NULL;
	char read_buf[1024];
	fp = fopen(config_name,"r");

	while(fgets(read_buf,1024,fp))
	{
		if(read_buf[0]=='#' || read_buf[0] == '\0' || read_buf[0] == ' ')
			continue;
		read_buf[strlen(read_buf)-2] = '\0';
		
		for(i = 0;i < 7;i++)
		{
			p=strstr(read_buf,objects[i]);
			if (p)
			{
				pos = p-read_buf+strlen(objects[i])+1;	
				strcpy(values[i],&read_buf[pos]);			
			}
		}
	}
	fclose(fp);
	return 0;
}
