#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define DHT11_DEV "/dev/mydht11"
#define DATA_SIZE 4

int main(int argc, char *argv[])
{
    int fd;
    unsigned char data[DATA_SIZE];
    int ret;
    float humidity, temperature;
    int loop_count = 0;
    int max_loops = 1;

    if (argc > 1) {
        max_loops = atoi(argv[1]);
        if (max_loops <= 0) {
            fprintf(stderr, "Usage: %s [loop_count]\n", argv[0]);
            return 1;
        }
    }

    fd = open(DHT11_DEV, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open " DHT11_DEV);
        fprintf(stderr, "Please ensure:\n");
        fprintf(stderr, "  1. DHT11 driver is loaded: insmod dht11_drv.ko\n");
        fprintf(stderr, "  2. Device node exists: ls -la %s\n", DHT11_DEV);
        fprintf(stderr, "  3. Permissions are correct: chmod 666 %s\n", DHT11_DEV);
        return 1;
    }

    printf("DHT11 Temperature and Humidity Monitor\n");
    printf("========================================\n\n");

    while (loop_count < max_loops) {
        ret = read(fd, data, DATA_SIZE);
        if (ret < 0) {
            perror("Failed to read data");
            close(fd);
            return 1;
        }

        if (ret != DATA_SIZE) {
            fprintf(stderr, "Unexpected data size: %d (expected %d)\n", ret, DATA_SIZE);
            close(fd);
            return 1;
        }

        humidity = data[0] + data[1] / 10.0f;
        temperature = data[2] + data[3] / 10.0f;

        printf("Temperature: %.1f¡ãC, Humidity: %.1f%%\n", temperature, humidity);

        loop_count++;

        if (loop_count < max_loops) {
            sleep(2);
        }
    }

    close(fd);
    return 0;
}