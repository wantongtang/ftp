#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include <pwd.h>
#include <grp.h>

void get_myls(int sock_fd)
{
	DIR *dir;
	struct dirent *entry;
	struct stat stat_buf;
	char buf[1024] = {0};
	char *path = "./";
	struct tm *tm_local;
	int year;
	time_t tm_now;
	time(&tm_now);
	tm_local = localtime(&tm_now);
	year = tm_local->tm_year;
	
//	printf("year now = %d\n", tm_local->tm_year);
	
	if ((dir = opendir(path)) == NULL)
	{
		fprintf(stderr, "cannot open directory: %s\n", path);
		return;
	}
	chdir(path);
	while ((entry = readdir(dir)) != NULL)
	{
		char popedom[11] = "----------";
		int pos = 0;
		char tm_buf[32] = {0};
		struct tm *tm_ptr;
		struct passwd *ps_ptr;
		struct group  *grp_ptr;

		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

/*
		//不显示隐藏文件
		if (entry->d_name[0] == '.')
		{
			continue;
		}
*/
		lstat(entry->d_name, &stat_buf);
	
		//文件类型
		switch ((stat_buf.st_mode & S_IFMT))
		{
		case S_IFSOCK:
			popedom[0] = 's';
			break;
		case S_IFLNK:
			popedom[0] = 'l';
			break;
		case S_IFREG:
			popedom[0] = '-';
			break;
		case S_IFBLK:
			popedom[0] = 'b';
			break;
		case S_IFDIR:
			popedom[0] = 'd';
			break;
		case S_IFCHR:
			popedom[0] = 'c';
			break;
		case S_IFIFO:
			popedom[0] = 'p';
			break;
		default:
			break;		
		}

		//拥有者权限
		if ( (stat_buf.st_mode & S_IRUSR) == S_IRUSR)
		{
			popedom[1] = 'r';
		}

		if ( (stat_buf.st_mode & S_IWUSR) == S_IWUSR)
		{
			popedom[2] = 'w';
		}

		if ( (stat_buf.st_mode & S_IXUSR) == S_IXUSR)
		{
			popedom[3] = 'x';
		}

		//组用户权限
		if ( (stat_buf.st_mode & S_IRGRP) == S_IRGRP)
		{
			popedom[4] = 'r';
		}

		if ( (stat_buf.st_mode & S_IWGRP) == S_IWGRP)
		{
			popedom[5] = 'w';
		}

		if ( (stat_buf.st_mode & S_IXGRP) == S_IXGRP)
		{
			popedom[6] = 'x';
		}

		//其他用户权限
		if ( (stat_buf.st_mode & S_IROTH) == S_IROTH)
		{
			popedom[7] = 'r';
		}

		if ( (stat_buf.st_mode & S_IWOTH) == S_IWOTH)
		{
			popedom[8] = 'w';
		}

		if ( (stat_buf.st_mode & S_IXOTH) == S_IXOTH)
		{
			popedom[9] = 'x';
		}

		pos += sprintf(buf, "%s ", popedom);		//文件类型及权限

		ps_ptr = getpwuid(stat_buf.st_uid);		
		grp_ptr = getgrgid(stat_buf.st_gid);	
		pos += sprintf(buf+pos, "%d %s %s %d ", \
			stat_buf.st_nlink, ps_ptr->pw_name, grp_ptr->gr_name, stat_buf.st_size);

/*
		pos += sprintf(buf+pos, "%d %d %d %u ", \
			stat_buf.st_nlink, stat_buf.st_uid, stat_buf.st_gid, stat_buf.st_size);
*/

		tm_ptr = localtime(&stat_buf.st_ctime);	
		
//		printf("local = %d\n", year);
//		printf("file = %d\n", tm_ptr->tm_year);

		if (year == tm_ptr->tm_year)
		{
			strftime(tm_buf, 32, "%b %d %Y,%H:%M", tm_ptr);
		}
		else 
		{
			strftime(tm_buf, 32, "%b %d %Y", tm_ptr);
		}

//		strftime(tm_buf, 32, "%Y-%m-%d,%H:%M", tm_ptr);
//		strftime(tm_buf, 32, "%b %d %Y,%H:%M", tm_ptr);

		pos += sprintf(buf+pos, "%s ", tm_buf);

		if (popedom[0] == 'l')
		{
			char linkname[32] = {0};
			readlink(entry->d_name, linkname, sizeof(linkname));
			pos += sprintf(buf+pos, "%s->%s\r\n", entry->d_name, linkname);
		}
		else
		{
			pos += sprintf(buf+pos, "%s\r\n", entry->d_name);
		}
		write(sock_fd, buf, strlen(buf));
//		printf("%s", buf);
		memset(buf, 0x00, sizeof(buf));		
	}

	closedir(dir);
}

