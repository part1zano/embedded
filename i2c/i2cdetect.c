#include <sys/cdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <dev/iicbus/iic.h>
 
int main ( int argc, char **argv )  {

	int i, fd;
	char dev[] = "/dev/iic1";
	uint8_t buf[2] = { 0, 0 };
	struct iic_msg msg[2];
	struct iic_rdwr_data rdwr;

	msg[0].flags = !IIC_M_RD;
	msg[0].len = sizeof(buf);
	msg[0].buf = buf;

	msg[1].flags = IIC_M_RD;
	msg[1].len = sizeof(buf);
	msg[1].buf = buf;

	rdwr.nmsgs = 2;

	if ( (fd = open(dev, O_RDWR)) < 0 )  {
		perror("open");
		exit(-1);
	}

	for ( i = 1; i < 128; i++ )  {
		msg[0].slave = i;
		msg[1].slave = i;
		rdwr.msgs = msg;
		if ( ioctl(fd, I2CRDWR, &rdwr) >= 0 )  printf( "%02x\n", i );
	}

	close(fd);
	exit(0);
}
