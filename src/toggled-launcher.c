#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>

/*
 * Comment CHANGE_LAUNCHER_ICON macro to use `toggled` only for
 * toggling services, i.e., disable checking XFCE panel launchers.
 */
#define CHANGE_LAUNCHER_ICON
#ifndef CHANGE_LAUNCHER_ICON
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define PANEL_PATH "~/.config/xfce4/panel/"
#define FIND_DIRECTIVE "Exec="
#define REPLACE_DIRECTIVE "Icon="
#define MAX_LINE_SIZE 1024


extern const char *g_cmd_name;


static int resolve_pathname(char *resolved_path)
{
	int status;

	FILE *pipe = popen("ls -d " PANEL_PATH " 2>/dev/null", "r");
	if (!pipe) {
		perror(__func__);
		return EPIPE;
	}

	if (fgets(resolved_path, FILENAME_MAX, pipe)) {
		resolved_path[strcspn(resolved_path, "\n")] = '\0';
	}

	status = pclose(pipe);
	if (status == -1) {
		perror(__func__);
		return EPIPE;

	} else if (status != 0) {
		status = WEXITSTATUS(status);

	}

	if (status == ENOENT) {
		fprintf(stderr,
			"\033[35mWarning: XFCE panel directory not found at \033[3m%s\n"
			"If XFCE not present, comment CHANGE_LAUNCHER_ICON in %s\033[m\n",
			PANEL_PATH, __FILE__);
	}

	return status;
}

static bool remove_extension(char *restrict str, const char *restrict ext)
{
	size_t i = 0;
	size_t size_str = 0;
	size_t size_ext = 0;

	while (str[size_str++])
		;
	while (ext[size_ext++])
		;
	size_str--;
	size_ext--;

	for (i = 0; size_ext - i; ++i) {
		if (str[size_str - i] != ext[size_ext - i]) {
			break;
		}
	}

	if (size_ext == i) {
		str[size_str - i] = '\0';

		return true;
	}

	return false;
}

static int modify_file_inplace(const char *filename, const char *toggle)
{
	FILE *fp = fopen(filename, "r+");
	if (fp == NULL) {
		perror(__func__);
		return errno;
	}

	char line[MAX_LINE_SIZE];
	size_t line_len = 0;
	off_t file_size = 0;

	char find_line[MAX_LINE_SIZE];
	char replace_line[MAX_LINE_SIZE];
	long size_diff = 0;

	long match_position = 0;
	long cur_position = 0;
	int c = 0;

	strcpy(find_line, REPLACE_DIRECTIVE);

	while (fgets(line, MAX_LINE_SIZE, fp)) {
		line_len = strlen(line);
		file_size += line_len;

		if (!strncmp(line, find_line, strlen(find_line))) {
			const char *extensions[] = {".svg", ".png", ".jpg", ".jpeg", "-on", "-off"};
			size_t ext_size = sizeof(extensions) / sizeof(extensions[0]);
			size_t idx = 0;

			strcpy(replace_line, line);
			remove_extension(replace_line, "\n");

			for (; idx < ext_size; ++idx) {
				if (remove_extension(replace_line, extensions[idx])) {
					break;
				}
			}

			if (idx < ext_size - 2) {
				if (!remove_extension(replace_line, "-on")) {
					remove_extension(replace_line, "-off");
				}

				strcat(replace_line, "-");
				strcat(replace_line, toggle);
				strcat(replace_line, extensions[idx]);
			} else {
				strcat(replace_line, "-");
				strcat(replace_line, toggle);
			}

			size_diff = strlen(replace_line) + 1 - line_len;
			file_size += size_diff;

			if (size_diff > 0) {
				match_position = ftell(fp);
				fseek(fp, -1L, SEEK_END);
				cur_position = ftell(fp);

				for (; match_position <= cur_position; --cur_position) {
					c = fgetc(fp);
					fseek(fp, size_diff - 1, SEEK_CUR);
					fputc(c, fp);
					fseek(fp, -size_diff - 2, SEEK_CUR);
				}

				fseek(fp, match_position - line_len, SEEK_SET);
				fputs(replace_line, fp);
				fputc('\n', fp);

			} else if (size_diff < 0) {
				fseek(fp, -line_len, SEEK_CUR);
				fputs(replace_line, fp);
				fputc('\n', fp);
				match_position = ftell(fp);

				fseek(fp, -size_diff, SEEK_CUR);

				while (fgets(line, MAX_LINE_SIZE, fp)) {
					fseek(fp, size_diff - strlen(line), SEEK_CUR);
					fputs(line, fp);
					fseek(fp, -size_diff, SEEK_CUR);
				}

				fseek(fp, match_position, SEEK_SET);

				// Reduce file size in advance to account for the extra
				// characters left at the end of the file due to shifting
				// the content toward the beginning of the file
				file_size += size_diff;

			} else {
				fseek(fp, -line_len, SEEK_CUR);
				fputs(replace_line, fp);
				fputc('\n', fp);
			}
		}
	}

	int fd = fileno(fp);
	if (fd == -1) {
		perror(__func__);
		fclose(fp);
		return errno;
	}

	int result = ftruncate(fd, file_size);
	if (result == -1) {
		perror(__func__);
		fclose(fp);
		return errno;
	}

	fclose(fp);

	return 0;
}

static inline int find_launcher(const char *filename, char *service, const char *toggle,
				bool (*callback)(const char*))
{
	FILE *fp = fopen(filename, "r+");
	if (fp == NULL) {
		perror(__func__);
		return errno;
	}

	char line[MAX_LINE_SIZE];

	size_t find_line_size = strlen(FIND_DIRECTIVE) + strlen(g_cmd_name) +
		strlen(service) + 2;
	char find_line[find_line_size];

	strcpy(find_line, FIND_DIRECTIVE);
	strcat(find_line, g_cmd_name);
	strcat(find_line, " ");
	strcat(find_line, service);

	while (fgets(line, MAX_LINE_SIZE, fp)) {
		if (!strncmp(line, find_line, strlen(find_line))) {
			fclose(fp);

			if (!strcmp(toggle, "--status")) {
				remove_extension(line, "\n");
				remove_extension(line, ".service");
				service = strrchr(line, ' ') + 1;

				if (callback(service)) {
					printf("\033[3m%-20s\033[23;1;32mON\033[m\n", service);
				} else {
					printf("\033[3m%-20s\033[23;1;31mOFF\033[m\n", service);
				}

				return 0;

			} else if (!strcmp(toggle, "--sync-icons")) {
				remove_extension(line, "\n");
				remove_extension(line, ".service");
				service = strrchr(line, ' ') + 1;

				if (callback(service)) {
					toggle = "on";
				} else {
					toggle = "off";
				}

			}

			return modify_file_inplace(filename, toggle);
		}

	}

	fclose(fp);

	return ENOENT;
}

/*
 * find_files() recursively checks all launchers on each available XFCE panel, even if the same
 * service has multiple launchers. It will continue searching for every launcher relevant to the
 * service, however, the first successful instance will determine the return value.
 */
static int find_files(const char *dir, char *service, const char *toggle,
			bool (*find_launcher_arg)(const char*))
{
	if (access(dir, R_OK)) {
		perror(__func__);
		return errno;
	}

	static int ret = ENOENT;
	int ret_tmp = ENOENT;

	struct dirent *dir_struct;
	DIR *dir_p = opendir(dir);
	chdir(dir);

	while ((dir_struct = readdir(dir_p)) != NULL) {
		switch (dir_struct->d_type) {
		case DT_DIR:
			if (strcmp(dir_struct->d_name, ".") && strcmp(dir_struct->d_name, "..")) {
				find_files(dir_struct->d_name, service, toggle, find_launcher_arg);
			}
			break;
		case DT_REG:
			ret_tmp = find_launcher(dir_struct->d_name, service,
						toggle, find_launcher_arg);

			ret = ret_tmp == 0 ? 0 : ret;
			break;
		}
	}

	chdir("..");
	closedir(dir_p);

	return ret;
}

int change_launcher_icon(char *service, const char *toggle, bool (*find_launcher_arg)(const char*))
{
	int ret = 0;

#ifdef CHANGE_LAUNCHER_ICON
	// Rely on shell to evaluate PANEL_PATH because it might
	// contain special shell variables, e.g., ~ or $HOME
	char resolved_panel_path[FILENAME_MAX];

	ret = resolve_pathname(resolved_panel_path);
	if (ret != 0) {
		return ret;
	}

	if (service) {
		remove_extension(service, ".service");
	} else {
		service = "";
	}

	ret = find_files(resolved_panel_path, service, toggle, find_launcher_arg);
#endif
	return ret;
}
