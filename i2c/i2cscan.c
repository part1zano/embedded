/*
 * i2cscan  FreeBSD version of i2cdetect
 *          Compile with:
 *              cc i2cscan.c -o i2cscan
 *
 * Winston Smith <smith.winston.101@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <dev/iicbus/iic.h>

/*
 * Scan for the specified slave by trying to read two bytes
 */
int scan(int fd, int slave)
{
	uint8_t buf[2], offset;
	struct iic_msg msg[2];
	struct iic_rdwr_data rdwr;

	offset = 0;
	msg[0].slave = slave << 1;
	msg[0].flags = IIC_M_WR;
	msg[0].len = sizeof(offset);
	msg[0].buf = &offset;

	msg[1].slave = slave << 1;
	msg[1].flags = IIC_M_RD;
	msg[1].len = sizeof(buf);
	msg[1].buf = buf;

	rdwr.nmsgs = 2;
	rdwr.msgs = msg;

	if (ioctl(fd, I2CRDWR, &rdwr) < 0) {
		switch (errno) {
		case EIO:
		case ENXIO:
		case ENOENT:
			// Doesn't exist
			return (1);
		case EBUSY:
			// Currently in use
			return (2);
		default:
			perror("ioctl(I2CRDWR) failed");
			return (-1);
		}
	}

	return (0);
}

int
main(int argc, char **argv)
{
	int addr, c, fd, r;
	char *dev = "/dev/iic0";

	if (argc > 1)
		dev = argv[1];
	if ((fd = open(dev, O_RDWR)) < 0) {
		perror("open failed");
		exit(-1);
	}

	printf("Checking device: %s\n", dev);
	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	for (r = 0; r < 8; r++) {
		printf("%02X:", r * 16);
		for (c = 0; c < 16; c++) {
			addr = r * 16 + c;
			switch (scan(fd, addr)) {
			case 0:
				printf(" %02X", addr);
				break;
			case 1:
				printf(" --");
				break;
			case 2:
				printf(" UU");
				break;
			default:
				break;
			}

		}
		printf("\n");
	}

	close(fd);
	exit(0);
}
