#include <stdio.h>
#include <sys/types.h>
#include <sys/sysctl.h>

int main(int argc, char **argv) {
	int bmp085temp;
	int bmp085pressure;
	int ds18b20temp;
	size_t len;
	len = sizeof(int);

	sysctlbyname("dev.bmp085.0.temperature", &bmp085temp, &len, NULL, 0);
	sysctlbyname("dev.bmp085.0.pressure", &bmp085pressure, &len, NULL, 0);
	sysctlbyname("dev.ow_temp.0.temperature", &ds18b20temp, &len, NULL, 0);
	printf("Current bmp085 temperature: %3.3f\n", (bmp085temp-2731.5f)/10.0f);
	printf("Current bmp085 pressure: %d\n", bmp085pressure);
	printf("Current ds18b20 temperature: %3.3f\n", (ds18b20temp-273150)/1000.0f);
	return 0;
}
