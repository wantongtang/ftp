.PHONY:clean
CC = gcc
OBJS = myftp.o ftp_sock.o ftp_cmd.o ftp_myls.o ftp_shm.o ftp_pub.o
LIBS = -lcrypt

myftp:$(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

$(OBJS):%.o:%.c
	$(CC) -c $< -o $@

clean:
	rm -f $(OBJS) myftp
