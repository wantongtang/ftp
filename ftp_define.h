#ifndef _FTP_DEFINE_H_
#define _FTP_DEFINE_H_

#define SHM_MAX 4096

enum trans_type{STOR = 1, APPE = 2, REST = 3};

typedef int (*op_fun)(void *data);

typedef struct op_cmd
{
	char	*cmd_key;
	op_fun	opfun;	
}OPCMD;

typedef struct cmd_info
{
	char	usrname[128];
	char	data[128];
	char	buf[128];
	int		acc_socket;
	int		data_socket;
	int		rein_flg;
//	int		trans_flg;
}CMDINFO;

typedef struct sh_stat
{
	int				stor_count;
	int				retr_count;
	unsigned long	stor_bytes;
	unsigned long	retr_bytes;
}SHSTAT;

union semun 
{
  int val;                  /* value for SETVAL */
  struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
  unsigned short *array;    /* array for GETALL, SETALL */
  struct seminfo *__buf;    /* buffer for IPC_INFO */
};

#endif /*_FTP_DEFINE_H_*/
