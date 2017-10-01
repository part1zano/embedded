#include <stdio.h>
#include <sys/sysctl.h>
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

	printf("Content-Type: text/html, charset=us-ascii\n\n");
	printf("<html><head><title>Temperature and pressure at home</title></head><body>\n");
	printf("<table><tr><td>Sensor</td><td>Temperature</td><td>Pressure</td></tr>\n");
	printf("<tr><td>bmp085 </td><td> %3.3f", (bmp085temp-2731.5f)/10.0f);
	printf("<td> %d</td></tr>\n", bmp085pressure);
	printf("<tr><td>ds18b20</td><td> %3.3f\n</td><td> ----- </td></tr>\n", (ds18b20temp-273150)/1000.0f);
	printf("</table>\n");
	printf("</html>");
	return 0;
}
