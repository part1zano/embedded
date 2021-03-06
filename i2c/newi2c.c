/*-
 * Copyright (C) 2008-2009 Semihalf, Michal Hajduk and Bartlomiej Sieka
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/usr.sbin/i2c/i2c.c 230356 2012-01-20 01:38:44Z eadler $");

#include <err.h>
#include <errno.h>
#include <sysexits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <dev/iicbus/iic.h>

#define	I2C_DEV			"/dev/iic0"
#define	I2C_MODE_NOTSET		0
#define	I2C_MODE_NONE		1
#define	I2C_MODE_STOP_START	2
#define	I2C_MODE_REPEATED_START	3

struct options {
	int	width;
	int	count;
	int	verbose;
	int	addr_set;
	int	binary;
	int	scan;
	int	skip;
	int	reset;
	int	mode;
	char	dir;
	uint16_t	addr;
	uint16_t	off;
};

struct skip_range {
	int	start;
	int	end;
};

__dead2 static void
usage(void)
{

	fprintf(stderr, "usage: %s -a addr [-f device] [-d [r|w]] [-o offset] "
	    "[-w [0|8|16]] [-c count] [-m [ss|rs|no]] [-b] [-v]\n",
	    getprogname());
	fprintf(stderr, "       %s -s [-f device] [-n skip_addr] -v\n",
	    getprogname());
	fprintf(stderr, "       %s -r [-f device] -v\n", getprogname());
	exit(EX_USAGE);
}

static struct skip_range
skip_get_range(char *skip_addr)
{
	struct skip_range addr_range;
	char *token;

	addr_range.start = 0;
	addr_range.end = 0;

	token = strsep(&skip_addr, "..");
	if (token) {
		addr_range.start = strtoul(token, 0, 16);
		token = strsep(&skip_addr, "..");
		if ((token != NULL) && !atoi(token)) {
			token = strsep(&skip_addr, "..");
			if (token)
				addr_range.end = strtoul(token, 0, 16);
		}
	}

	return (addr_range);
}

/* Parse the string to get hex 7 bits addresses */
static int
skip_get_tokens(char *skip_addr, int *sk_addr, int max_index)
{
	char *token;
	int i;

	for (i = 0; i < max_index; i++) {
		token = strsep(&skip_addr, ":");
		if (token == NULL)
			break;
		sk_addr[i] = strtoul(token, 0, 16);
	}
	return (i);
}

static int
scan_bus(struct iiccmd cmd, char *dev, int skip, char *skip_addr)
{
	struct skip_range addr_range = { 0, 0 };
	int *tokens, fd, i, index, j;
	int len = 0, do_skip = 0, no_range = 1;
	struct iic_msg msg[2];
	struct iic_rdwr_data rdwr;
	int mute;
	uint8_t offset, buf;
	int found = 0;

	fd = open(dev, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "Error opening I2C controller (%s) for "
		    "scanning: %s\n", dev, strerror(errno));
		return (EX_NOINPUT);
	}

	if (skip) {
		len = strlen(skip_addr);
		if (strstr(skip_addr, "..") != NULL) {
			addr_range = skip_get_range(skip_addr);
			no_range = 0;
		} else {
			tokens = (int *)malloc((len / 2 + 1) * sizeof(int));
			if (tokens == NULL) {
				fprintf(stderr, "Error allocating tokens "
				    "buffer\n");
				goto out;
			}
			index = skip_get_tokens(skip_addr, tokens,
			    len / 2 + 1);
		}

		if (!no_range && (addr_range.start > addr_range.end)) {
			fprintf(stderr, "Skip address out of range\n");
			goto out;
		}
	}

/* To read from non existing device we need first to disable console messages */
	mute = 1;
	sysctlbyname("kern.consmute", NULL, NULL, &mute, sizeof(mute) );

	printf("Scanning I2C devices on %s: ", dev);
	for (i = 1; i < 127; i++) {

		if (skip && ( addr_range.start < addr_range.end)) {
			if (i >= addr_range.start && i <= addr_range.end)
				continue;

		} else if (skip && no_range)
			for (j = 0; j < index; j++) {
				if (tokens[j] == i) {
					do_skip = 1;
					break;
				}
			}

		if (do_skip) {
			do_skip = 0;
			continue;
		}

		offset = 0;
		msg[0].slave = i << 1 | !IIC_M_RD;
		msg[0].flags = !IIC_M_RD;
		msg[0].len = sizeof( offset );
		msg[0].buf = &offset;
		msg[1].slave = i << 1 | IIC_M_RD;
		msg[1].flags = IIC_M_RD;
		msg[1].len = sizeof( buf );
		msg[1].buf = &buf;
		rdwr.msgs = msg;
		rdwr.nmsgs = 2;
		if ( ioctl(fd, I2CRDWR, &rdwr) >= 0 )  {
		  printf("%x ", i);
		  found = 1;
		}
	
	}

	if ( !found )  printf( "nothing found\n" );

/* Now we need to enable console messages */
	mute = 0;
	sysctlbyname("kern.consmute", NULL, NULL, &mute, sizeof(mute) );
	printf("\n");

out:
	close(fd);
	if (skip && no_range)
		free(tokens);

	return (EX_OK);
}

static int
reset_bus(struct iiccmd cmd, char *dev)
{

/* We can't reset bus, so do nothing */

	return (EX_OK);
}

static uint8_t *
prepare_buf(int size, uint32_t off)
{
	uint8_t *buf;

	buf = malloc(size);
	if (buf == NULL)
		return (buf);

	if (size == 1)
		buf[0] = off & 0xff;
	else if (size == 2) {
		buf[0] = (off >> 8) & 0xff;
		buf[1] = off & 0xff;
	}

	return (buf);
}

static int
i2c_write(char *dev, struct options i2c_opt, uint8_t *i2c_buf)
{
	int ch, i, fd, bufsize;
	char *err_msg;
	struct iic_msg msg;
	struct iic_rdwr_data rdwr;
	uint8_t *buf, *dbuf;
	

	/*
	 * Read data to be written to the chip from stdin
	 */
	if (i2c_opt.verbose && !i2c_opt.binary)
		fprintf(stderr, "Enter %u bytes of data: ", i2c_opt.count);

	for (i = 0; i < i2c_opt.count; i++) {
		ch = getchar();
		if (ch == EOF) {
			free(i2c_buf);
			err(1, "not enough data, exiting\n");
		}
		i2c_buf[i] = (uint8_t)ch;
	}

	fd = open(dev, O_RDWR);
	if (fd == -1) {
		free(i2c_buf);
		err(1, "open failed");
	}

	/*
	 * Write offset where the data will go
	 */

	if (i2c_opt.width) {
		bufsize = i2c_opt.width / 8;
		buf = prepare_buf(bufsize, i2c_opt.off);
		if (buf == NULL) {
			err_msg = "error: offset malloc";
			goto err1;
		}
	}

	/*
	 * Write the data
	 */
	dbuf = malloc( (bufsize + i2c_opt.count) * sizeof(uint8_t) );

	for (i = 0; i < bufsize; i++) {
	  dbuf[i] = buf[i];
	}

	for (i = 0; i < i2c_opt.count; i++ )  {
	  dbuf[i+bufsize] = i2c_buf[i];
	}

	msg.slave = i2c_opt.addr << 1 | !IIC_M_RD;
	msg.flags = !IIC_M_RD;
	msg.len = (bufsize + i2c_opt.count) * sizeof( uint8_t );
	msg.buf = dbuf;
	rdwr.msgs = &msg;
	rdwr.nmsgs = 1;
	if ( ioctl(fd, I2CRDWR, &rdwr) < 0 )  {
	  err_msg = "ioctl: error when write";
	  goto err1;
	}

	close(fd);
	return (0);

err1:
	close(fd);
	return (1);
}

static int
i2c_read(char *dev, struct options i2c_opt, uint8_t *i2c_buf)
{
	int i, fd, bufsize;
	char *err_msg;
	struct iic_msg *msg;
	struct iic_rdwr_data rdwr;
	uint8_t *buf;

	fd = open(dev, O_RDWR);
	if (fd == -1)
		err(1, "open failed");

	if (i2c_opt.width) {
		bufsize = i2c_opt.width / 8;
		buf = prepare_buf(bufsize, i2c_opt.off);
		if (buf == NULL) {
			err_msg = "error: offset malloc";
			goto err1;
		}
	}

	msg = malloc( (bufsize + i2c_opt.count) * sizeof(struct iic_msg) );

	for (i = 0; i < bufsize; i++) {
	  msg[i].slave = i2c_opt.addr << 1 | !IIC_M_RD;
	  msg[i].flags = !IIC_M_RD;
	  msg[i].len = sizeof( uint8_t );
	  msg[i].buf = &buf[i];
	}

	for (i = 0; i < i2c_opt.count; i++ )  {
	  msg[i+bufsize].slave = i2c_opt.addr << 1 | IIC_M_RD;
	  msg[i+bufsize].flags = IIC_M_RD;
	  msg[i+bufsize].len = sizeof( uint8_t );
	  msg[i+bufsize].buf = &i2c_buf[i];
	}

	rdwr.msgs = msg;
	rdwr.nmsgs = bufsize + i2c_opt.count;

	if ( ioctl(fd, I2CRDWR, &rdwr) < 0 )  {
	  err_msg = "ioctl: error while reading";
	  goto err1;
	}
	
	close(fd);
	return (0);

err1:
	close(fd);
	return (1);
}

int
main(int argc, char** argv)
{
	struct iiccmd cmd;
	struct options i2c_opt;
	char *dev, *skip_addr;
	int error, chunk_size, i, j, ch;
	uint8_t *i2c_buf;

	errno = 0;
	error = 0;

	/* Line-break the output every chunk_size bytes */
	chunk_size = 16;

	dev = I2C_DEV;

	/* Default values */
	i2c_opt.addr_set = 0;
	i2c_opt.off = 0;
	i2c_opt.verbose = 0;
	i2c_opt.dir = 'r';	/* direction = read */
	i2c_opt.width = 8;
	i2c_opt.count = 1;
	i2c_opt.binary = 0;	/* ASCII text output */
	i2c_opt.scan = 0;	/* no bus scan */
	i2c_opt.skip = 0;	/* scan all addresses */
	i2c_opt.reset = 0;	/* no bus reset */
	i2c_opt.mode = I2C_MODE_NOTSET;

	while ((ch = getopt(argc, argv, "a:f:d:o:w:c:m:n:sbvrh")) != -1) {
		switch(ch) {
		case 'a':
			sscanf( optarg, "%hX", &i2c_opt.addr);
			if (i2c_opt.addr == 0 && errno == EINVAL)
				i2c_opt.addr_set = 0;
			else
				i2c_opt.addr_set = 1;
			break;
		case 'f':
			dev = optarg;
			break;
		case 'd':
			i2c_opt.dir = optarg[0];
			break;
		case 'o':
			i2c_opt.off = strtoul(optarg, 0, 16);
			if (i2c_opt.off == 0 && errno == EINVAL)
				error = 1;
			break;
		case 'w':
			i2c_opt.width = atoi(optarg);
			break;
		case 'c':
			i2c_opt.count = atoi(optarg);
			break;
		case 'm':
			if (!strcmp(optarg, "no"))
				i2c_opt.mode = I2C_MODE_NONE;
			else if (!strcmp(optarg, "ss"))
				i2c_opt.mode = I2C_MODE_STOP_START;
			else if (!strcmp(optarg, "rs"))
				i2c_opt.mode = I2C_MODE_REPEATED_START;
			else
				usage();
			break;
		case 'n':
			i2c_opt.skip = 1;
			skip_addr = optarg;
			break;
		case 's':
			i2c_opt.scan = 1;
			break;
		case 'b':
			i2c_opt.binary = 1;
			break;
		case 'v':
			i2c_opt.verbose = 1;
			break;
		case 'r':
			i2c_opt.reset = 1;
			break;
		case 'h':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/* Set default mode if option -m is not specified */
	if (i2c_opt.mode == I2C_MODE_NOTSET) {
		if (i2c_opt.dir == 'r')
			i2c_opt.mode = I2C_MODE_STOP_START;
		else if (i2c_opt.dir == 'w')
			i2c_opt.mode = I2C_MODE_NONE;
	}

	/* Basic sanity check of command line arguments */
	if (i2c_opt.scan) {
		if (i2c_opt.addr_set)
			usage();
	} else if (i2c_opt.reset) {
		if (i2c_opt.addr_set)
			usage();
	} else if (error) {
		usage();
	} else if ((i2c_opt.dir == 'r' || i2c_opt.dir == 'w')) {
		if ((i2c_opt.addr_set == 0) ||
		    !(i2c_opt.width == 0 || i2c_opt.width == 8 ||
		    i2c_opt.width == 16))
		usage();
	}

	if (i2c_opt.verbose)
		fprintf(stderr, "dev: %s, addr: 0x%x, r/w: %c, "
		    "offset: 0x%02x, width: %u, count: %u\n", dev,
		    i2c_opt.addr, i2c_opt.dir, i2c_opt.off,
		    i2c_opt.width, i2c_opt.count);

	if (i2c_opt.scan)
		exit(scan_bus(cmd, dev, i2c_opt.skip, skip_addr));

	if (i2c_opt.reset)
		exit(reset_bus(cmd, dev));

	i2c_buf = malloc(i2c_opt.count);
	if (i2c_buf == NULL)
		err(1, "data malloc");

	if (i2c_opt.dir == 'w') {
		error = i2c_write(dev, i2c_opt, i2c_buf);
		if (error) {
			free(i2c_buf);
			return (1);
		}
	}
	if (i2c_opt.dir == 'r') {
		error = i2c_read(dev, i2c_opt, i2c_buf);
		if (error) {
			free(i2c_buf);
			return (1);
		}
	}

	i = 0;
	j = 0;
	while (i < i2c_opt.count) {
		if (i2c_opt.verbose || (i2c_opt.dir == 'r' &&
		    !i2c_opt.binary))
			fprintf (stderr, "%02hhx ", i2c_buf[i++]);

		if (i2c_opt.dir == 'r' && i2c_opt.binary) {
			fprintf(stdout, "%c", i2c_buf[j++]);
			if(!i2c_opt.verbose)
				i++;
		}
		if (!i2c_opt.verbose && (i2c_opt.dir == 'w'))
			break;
		if ((i % chunk_size) == 0)
			fprintf(stderr, "\n");
	}
	if ((i % chunk_size) != 0)
		fprintf(stderr, "\n");

	free(i2c_buf);
	return (0);
}
