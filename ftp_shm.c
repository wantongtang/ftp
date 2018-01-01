#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "ftp_define.h"

/*初始化信号量*/
void init_sem(int sem_id)
{
	union semun sem_un;
	sem_un.val = 1;
	if (semctl(sem_id, 0, SETVAL, sem_un) < 0)
	{
		perror("semctl error:");	
	}
}

/*信号量P操作*/
void sem_p(int sem_id)
{
	struct sembuf sem_buf;
	sem_buf.sem_num = 0;
	sem_buf.sem_op = -1;
	sem_buf.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_buf, 1) != 0)
	{
		perror("semop p error:");
	}
}

/*信号量V操作*/
void sem_v(int sem_id)
{
	struct sembuf sem_buf;
	sem_buf.sem_num = 0;
	sem_buf.sem_op = 1;
	sem_buf.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_buf, 1) != 0)
	{
		perror("semop v error:");
	}
}

/*创建共享内存*/
int create_shm()
{
	int shmid;
	int semid;
	void *shm_addr = NULL;
	SHSTAT *stat_shm = NULL;

	while ((shmid = shmget((key_t)3344, SHM_MAX, IPC_CREAT|0777)) == 0)
	{
		free_shm_stat();
	}

	if (shmid < 0)
	{
		perror("create shm error:");
		return 0;
	}

	while ((semid = semget((key_t)5566, 1, IPC_CREAT|0777)) == 0)
	{
		free_sem();
	}

	if (semid < 0)
	{
		perror("sem create error:");
		return 0;
	}

	shm_addr = shmat(shmid, NULL, 0);
	if (shm_addr == NULL)
	{
		perror("shmat error:");
		return 0;
	}

	stat_shm = (SHSTAT *)shm_addr;
	stat_shm->stor_count = 0;
	stat_shm->stor_bytes = 0;

	stat_shm->retr_count = 0;
	stat_shm->retr_bytes = 0;

	if (shmdt(shm_addr)<0)
	{
		perror("shmdt error:");
		return 0;
	} 

	return 1;
}

/*获取共享内存中的统计数据*/
int get_shm_content(void *data)
{
	int shmid;
	int semid;
	void *shm_addr = NULL;
	SHSTAT *stat_shm = NULL;
	SHSTAT *stat = (SHSTAT *)data;

	shmid = shmget((key_t)3344, SHM_MAX, IPC_CREAT|0777);
	if (shmid < 0)
	{
		perror("create shm error:");
		return 0;
	}

	semid = semget((key_t)5566, 1, IPC_CREAT|0777);
	if (semid < 0)
	{
		perror("sem create error:");
		return 0;
	}

	shm_addr = shmat(shmid, NULL, 0);
	
	if (shm_addr == NULL)
	{
		perror("shmat error:");
		return 0;
	}
	
	init_sem(semid);
	stat_shm = (SHSTAT *)shm_addr;
	
	sem_p(semid);
	stat->stor_count = stat_shm->stor_count;
	stat->stor_bytes = stat_shm->stor_bytes;
	stat->retr_count = stat_shm->retr_count;
	stat->retr_bytes = stat_shm->retr_bytes;
	sem_v(semid);
/*	
	printf("stor_count = %d\n", stat_shm->stor_count);
	printf("stor_bytes = %d\n", stat_shm->stor_bytes);
	printf("retr_count = %d\n", stat_shm->retr_count);
	printf("retr_count = %d\n", stat_shm->retr_bytes);
*/
	if (shmdt(shm_addr)<0)
	{
		perror("shmdt error:");
		return 0;
	} 

	return 1;
}

/*修改共享内存中的统计数据*/
int modify_shm_stat(int flg, unsigned long bytes)
{
	int shmid;
	int semid;
	void *shm_addr = NULL;
	SHSTAT *stat_shm = NULL;

	shmid = shmget((key_t)3344, SHM_MAX, IPC_CREAT|0777);
	if (shmid < 0)
	{
		perror("create shm error:");
		return 0;
	}

	semid = semget((key_t)5566, 1, IPC_CREAT|0777);
	if (semid < 0)
	{
		perror("sem create error:");
		return 0;
	}

	shm_addr = shmat(shmid, NULL, 0);
	if (shm_addr == NULL)
	{
		perror("shmat error:");
		return 0;
	}

	init_sem(semid);
	stat_shm = (SHSTAT *)shm_addr;

	sem_p(semid);
	if (flg == 0)
	{
		stat_shm->stor_count++;
		stat_shm->stor_bytes += bytes/1024;
	}
	else if (flg == 1)
	{
		stat_shm->retr_count++;
		stat_shm->retr_bytes += bytes/1024;
	}
	sem_v(semid);

	if (shmdt(shm_addr)<0)
	{
		perror("shmdt error:");
		return 0;
	} 

	return 1;
}

/*释放共享内存*/
int free_shm_stat()
{
	int shmid;
	struct shmid_ds shm_ds; 

	shmid = shmget((key_t)3344, SHM_MAX, IPC_CREAT|0777);
	if (shmid < 0)
	{
		perror("get shm error:");
		return 0;
	}

	if (shmctl(shmid, IPC_RMID, &shm_ds) == -1)
	{
		perror("free shm error:");
		return 0;
	}

	return 1;
}

/*释放信号量*/
int free_sem()
{
	int semid;
	union semun sem_un;
	
	semid = semget((key_t)5566, 1, IPC_CREAT|0777);
	if (semid < 0)
	{
		perror("sem create error:");
		return 0;
	}

	if (semctl(semid, 0, IPC_RMID, sem_un) == -1)
	{
		perror("free sem error:");
		return 0;
	}

	return 1;
}

