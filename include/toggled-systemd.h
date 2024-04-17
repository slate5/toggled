#pragma once
#include <stdbool.h>

int toggle_service(const char *service, const char *method);
int check_result(const char *service);
bool is_service_available(const char *service);
bool is_service_active(const char *service);
