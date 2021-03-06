#include <stdio.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <string.h>

int main(int argc, char **argv) {
	int bmp085temp;
	int bmp085pressure;
	int ds18b20temp;
	char *data;
	unsigned short do_json=0;
	size_t len = sizeof(int);
	
	int refresh_time = 0; // don't refresh at all
	int say_kurwa = 0;

	sysctlbyname("dev.bmp085.0.temperature", &bmp085temp, &len, NULL, 0);
	sysctlbyname("dev.bmp085.0.pressure", &bmp085pressure, &len, NULL, 0);
	sysctlbyname("dev.ow_temp.0.temperature", &ds18b20temp, &len, NULL, 0);
	

	data = getenv("QUERY_STRING");
	if (data != NULL) {
		if (strstr(data, "json") != NULL) {
			do_json = 1;
		}
		if (sscanf(data, "refresh=%d", &refresh_time) != 1) {
			// say_kurwa = 1;
		}
		if (sscanf(data, "say_kurwa=%d", &say_kurwa) != 1) {
			say_kurwa = 1;
		}
	}
	if (do_json) {
		printf("Content-Type: application/json, charset=utf-8\n\n");

		printf("{\"bmp085\": {\"pressure\": %d, \"temperature\": %3.3f}, \"ds18b20\": {\"temperature\": %3.3f}}",
				bmp085pressure,
				(bmp085temp-2731.5f)/10.0f,
				(ds18b20temp-273150)/1000.0f
				);
	} else {
		printf("Content-Type: text/html, charset=utf-8\n\n");
		if (say_kurwa) {
			printf("\n\n\n<!-- KURWA TWOJA MAC!!! --->\n");
			printf("\n<!-- %s -->\n\n\n", strstr(data, "refresh"));
		}
		
		if (refresh_time) {
			printf("<meta http-equiv=\"refresh\" content=\"%d\" />\n", refresh_time);
		}
		printf("<html><head><title>Temperature and pressure at home</title></head><body>\n");
		printf("<center><table width=\"%s\"><tr><td>Sensor</td><td>Temperature</td><td>Pressure</td><td>Humidity</td></tr>\n", "30%");

		printf("<tr><td>bmp085 </td><td> %3.3f", (bmp085temp-2731.5f)/10.0f);
		printf("<td> %d</td><td>-----</td></tr>\n", bmp085pressure);
		printf("<tr><td>ds18b20</td><td> %3.3f\n</td><td> ----- </td><td>-----</td></tr>\n", (ds18b20temp-273150)/1000.0f);

		printf("</table></center>\n");
		printf("</html>");

	}

		return 0;
}
