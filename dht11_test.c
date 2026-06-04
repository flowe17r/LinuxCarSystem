#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/*
 * 用法: ./dht11_test /dev/mydht11
 */
int main(int argc, char **argv)
{
	int fd;
	unsigned char data[4];

	if (argc != 2) {
		printf("Usage: %s /dev/mydht11\n", argv[0]);
		return -1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("open");
		return -1;
	}

	while (1) {
		memset(data, 0, sizeof(data));
		if (read(fd, data, 4) == 4) {
			printf("humidity: %d.%d%%, temperature: %d.%d C\n",
			       data[0], data[1], data[2], data[3]);
		} else {
			printf("read failed\n");
		}
		sleep(2);
	}

	close(fd);
	return 0;
}
