#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "toggled-systemd.h"
#include "toggled-launcher.h"

#define HELP_LINE_MAX_LEN 80


const char *g_cmd_name;


static int help_txt();


int main(int argc, char *argv[])
{
	g_cmd_name = argv[0];
	char *service_name = NULL;
	char *toggle = NULL;
	int status;
	uid_t euid = geteuid();
	setreuid(euid, euid);

	switch (argc) {
	case 1:
		fprintf(stderr, "\033[1;31mService name is missing\033[m\n");

		return EINVAL;
	case 2:
		if (!strcmp(argv[1], "-h") || !strncmp(argv[1], "--help", strlen(argv[1]))) {
			return help_txt();

		} else if (!strcmp(argv[1], "--sync-icons") || !strcmp(argv[1], "--status")) {

			return change_launcher_icon(NULL, argv[1], is_service_active);
		}

		service_name = argv[1];

		break;
	case 3:
		if (!strcmp(argv[1], "on") || !strcmp(argv[1], "off")) {
			toggle = argv[1];
			service_name = argv[2];

		} else if (!strcmp(argv[2], "on") || !strcmp(argv[2], "off")) {
			toggle = argv[2];
			service_name = argv[1];

		} else {
			fprintf(stderr,
				"\033[1;31mExpected \033[3mon\033[23m or \033[3m"
				"off\033[23m as the optional argument\033[m\n");

			return EINVAL;
		}

		break;
	default:
		fprintf(stderr, "\033[1;31mToo many arguments provided\033[m\n\n");
		help_txt();

		return EINVAL;
	}

	if (!is_service_available(service_name)) {
		return ENOTSUP;
	}

	if (!toggle) {
		if (is_service_active(service_name)) {
			toggle = "off";
		} else {
			toggle = "on";
		}
	}

	if (strcmp(toggle, "on")) {
		status = toggle_service(service_name, "StopUnit");
	} else {
		status = toggle_service(service_name, "StartUnit");
	}

	if (status == 0) {
		status = check_result(service_name);

		if (status == 0) {
			status = change_launcher_icon(service_name, toggle, NULL);

			if (status == 0 || status == ENOENT) {
				printf("Service \033[3m%s\033[23m is turned %s\n",
					service_name, toggle);

				status = 0;
			} else {
				fprintf(stderr,
					"Service \033[3m%s\033[23m is turned %s, "
					"but changing the launcher icon failed\n",
					service_name, toggle);
			}
		} else {
			change_launcher_icon(service_name, "off", NULL);
		}
	}

	return status;
}

static int help_txt()
{
	const char txt[][HELP_LINE_MAX_LEN] = {
		"Usage:",
		"  toggled service [on|off]",
		"",
		"Arguments:",
		"  service             The name of the service to toggle.",
		"  [on|off]            Specify whether to toggle the service on or off.",
		"",
		"Standalone options:",
		"  --status            Print status of the services connected to the launchers.",
		"  --sync-icons        Sync all active launchers with service status.",
		"  -h, --help          Display this information.",
		"",
		"Note:",
		"  The order of arguments is not important, but autocomplete",
		"  suggests services first and then optional [on|off]."
	};

	for (size_t i = 0; i < sizeof(txt) / HELP_LINE_MAX_LEN; ++i) {
		printf("%s\n", txt[i]);
	}

	return 0;
}
