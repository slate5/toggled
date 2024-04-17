#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <systemd/sd-bus.h>

// Ignore errno format specifier `%m`
#pragma GCC diagnostic ignored "-Wformat"

#define _cleanup_(f) __attribute__((cleanup(f)))

#define DESTINATION "org.freedesktop.systemd1"
#define PATH        "/org/freedesktop/systemd1"
#define INTERFACE_M "org.freedesktop.systemd1.Manager"
#define INTERFACE_U "org.freedesktop.systemd1.Unit"
#define INTERFACE_S "org.freedesktop.systemd1.Service"


static void free_char_p(char **p)
{
	if (*p) {
		free(*p);
	}
}

static char *append_extension(const char *service)
{
	const char *ext = ".service";
	char *service_extended;

	if (!strcmp(service + strlen(service) - strlen(ext), ext)) {
		service_extended = malloc(strlen(service) + 1);
		strcpy(service_extended, service);
	} else {
		service_extended = malloc(strlen(service) + strlen(ext) + 1);
		strcpy(service_extended, service);
		strcat(service_extended, ext);
	}

	return service_extended;
}

static int log_error(int error, const char *format, ...)
{
	va_list args;
	errno = abs(error);
	const char *color_start = "\033[1;31m";
	const char *color_stop = "\033[m";
	char format_red[strlen(format) + strlen(color_start) + strlen(color_stop) + 2];

	snprintf(format_red, sizeof(format_red), "%s%s%s\n", color_start, format, color_stop);

	va_start(args, format);

	vfprintf(stderr, format_red, args);

	va_end(args);

	return errno;
}

int toggle_service(const char *service, const char *method)
{
	_cleanup_(sd_bus_flush_close_unrefp) sd_bus *bus = NULL;
	_cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
	_cleanup_(free_char_p) char *service_extended = append_extension(service);
	int ret;

	ret = sd_bus_open_system(&bus);
	if (ret < 0) {
		return log_error(ret, "Failed to acquire bus: %m");
	}

	ret = sd_bus_call_method(bus, DESTINATION, PATH, INTERFACE_M, method,
				&error, NULL, "ss", service_extended, "replace");
	if (ret < 0) {
		return log_error(ret, "%s failed: %s", method, error.message);
	}

	return 0;
}

int check_result(const char *service)
{
	_cleanup_(sd_bus_flush_close_unrefp) sd_bus *bus = NULL;
	_cleanup_(sd_bus_message_unrefp) sd_bus_message *reply = NULL;
	_cleanup_(free_char_p) char *service_extended = append_extension(service);
	const char *unit_file_path;
	_cleanup_(free_char_p) char *result_property = NULL;
	int ret;

	ret = sd_bus_open_system(&bus);
	if (ret < 0) {
		return log_error(ret, "Failed to acquire bus: %m");
	}

	ret = sd_bus_call_method(bus, DESTINATION, PATH, INTERFACE_M, "GetUnit",
				NULL, &reply, "s", service_extended);

	// If the service is not loaded, GetUnit will fail. In the context of checking
	// the result, it just means that the service was stopped, therefore `return 0`
	if (ret < 0) {
		return 0;
	}

	ret = sd_bus_message_read(reply, "o", &unit_file_path);
	if (ret < 0) {
		return log_error(ret, "Failed to get %s's file path: %m", service);
	}

	ret = sd_bus_get_property_string(bus, DESTINATION, unit_file_path, INTERFACE_S,
					"Result", NULL, &result_property);
	if (ret < 0) {
		return log_error(ret,
				"Failed to get %s's \033[3mResult\033[23m property: %m",
				service);

	} else if (!strcmp(result_property, "start-limit-hit")) {
		return log_error(EWOULDBLOCK, "%s got out-toggled: %s", service, result_property);

	} else if (strcmp(result_property, "success")) {
		return log_error(EPERM, "Failed: %s", result_property);

	}

	return 0;
}

bool is_service_available(const char *service)
{
	_cleanup_(sd_bus_flush_close_unrefp) sd_bus *bus = NULL;
	_cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
	_cleanup_(free_char_p) char *service_extended = append_extension(service);
	int ret;

	ret = sd_bus_open_system(&bus);
	if (ret < 0) {
		log_error(ret, "Failed to acquire bus: %m");
		return false;
	}

	ret = sd_bus_call_method(bus, DESTINATION, PATH, INTERFACE_M, "GetUnitFileState",
				&error, NULL, "s", service_extended);

	if (ret < 0) {
		log_error(ret, "Service %s unavailable: %s", service, error.message);
		return false;
	}

	return true;
}

bool is_service_active(const char *service)
{
	_cleanup_(sd_bus_flush_close_unrefp) sd_bus *bus = NULL;
	_cleanup_(sd_bus_message_unrefp) sd_bus_message *reply = NULL;
	_cleanup_(free_char_p) char *service_extended = append_extension(service);
	const char *unit_file_path;
	_cleanup_(free_char_p) char *active_state_property = NULL;
	int ret;

	ret = sd_bus_open_system(&bus);
	if (ret < 0) {
		log_error(ret, "Failed to acquire bus: %m");
		return false;
	}

	ret = sd_bus_call_method(bus, DESTINATION, PATH, INTERFACE_M, "GetUnit",
				NULL, &reply, "s", service_extended);
	if (ret < 0) {
		return false;
	}

	ret = sd_bus_message_read(reply, "o", &unit_file_path);
	if (ret < 0) {
		log_error(ret, "Failed to get %s's file path: %m", service);
		return false;
	}

	ret = sd_bus_get_property_string(bus, DESTINATION, unit_file_path, INTERFACE_U,
					"ActiveState", NULL, &active_state_property);
	if (ret < 0) {
		log_error(ret,
			"Failed to get %s's \033[3mActiveState\033[23m property: %m",
			service);
		return false;

	} else if (strcmp(active_state_property, "active")) {
		return false;

	}

	return true;
}
