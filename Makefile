# Usage:
# make [compile]             compile TARGET (default)
# make install               install TARGET and its utilities
# make remove                uninstall TARGET and its utilities
# make SERVICE=<all|name>    specify service (or `all`) to avoid being prompted
# make y                     proceed without prompting before copying or removing
#
# Note:
# `all` in SERVICE= means that all services that have a desktop entry and icons in
# etc/ of this directory will be installed.
#
# Example:
# make install y SERVICE=all

SHELL := /bin/bash
CC := gcc
CFLAGS := -I include/ -Wall -Wextra -pedantic
TARGET := toggled
SRCS := $(wildcard *.c src/*.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))
# This Makefile is expected to be executed with root privileges, so
# this finds the real user's home to install desktop applications there
REPO_DIR := $(abspath $(dir $(MAKEFILE_LIST)))
REAL_USER := $(shell logname)
REAL_USER_HOME := "$(shell getent passwd $(REAL_USER) | cut -d: -f6)"

INPUT_PS := "$(shell printf '\001\033[1;35m\002Enter service or \
			$(filter install remove,$(MAKECMDGOALS)) \
			\001\033[3m\002all\001\033[23m\002:\001\033[m\002 ')"

ifneq ($(shell command -v pkg-config 2>/dev/null),)
LIBS := $(shell pkg-config --silence-errors --libs libsystemd)
else
LIBS := $(shell gcc -fsyntax-only src/toggled-systemd.c 2>/dev/null && printf -- -lsystemd)
endif
ifeq ($(filter y,$(MAKECMDGOALS)),y)
CP_CMD := cp -f
RM_CMD := rm -f
else
CP_CMD := - cp -i
RM_CMD := - rm -i
endif

ifneq ($(XDG_CURRENT_DESKTOP),XFCE)
$(warning This tool primarily targets XFCE desktop enviroment...)
endif

ifeq ($(LIBS),)
$(error `libsystemd` not found. Please make sure `libsystemd-dev` is installed.)
endif

.PHONY: compile install _install remove _remove _clean _get_input_and_run _check_permission y SERVICE=

compile: $(TARGET) _clean

$(TARGET): $(OBJS)
	$(info Linking $(TARGET)...)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
	$(info Compiling $*.c...)
	$(CC) $(CFLAGS) -c $< -o $@

install: _check_permission compile _get_input_and_run
remove: _check_permission _get_input_and_run

_install:
	$(info Installing $(SERVICE)...)
	mkdir -p $(REAL_USER_HOME)/.local/share/applications/
	mkdir -p $(REAL_USER_HOME)/.local/share/icons/
	mkdir -p $(REAL_USER_HOME)/.config/autostart/
	chown root:$(REAL_USER) $(TARGET)
	chmod u+s $(TARGET)
	mv $(TARGET) /usr/local/bin/
	$(CP_CMD) $(REPO_DIR)/etc/$(TARGET) /usr/share/bash-completion/completions/
	$(CP_CMD) $(REPO_DIR)/etc/$(TARGET).desktop $(REAL_USER_HOME)/.config/autostart/
ifeq ($(SERVICE), all)
	$(CP_CMD) $(REPO_DIR)/etc/$(TARGET)-*.svg $(REAL_USER_HOME)/.local/share/icons/
	$(CP_CMD) $(REPO_DIR)/etc/$(TARGET)-*.desktop $(REAL_USER_HOME)/.local/share/applications/
else
	$(CP_CMD) $(REPO_DIR)/etc/$(TARGET)-$(SERVICE)*.svg $(REAL_USER_HOME)/.local/share/icons/
	$(CP_CMD) $(REPO_DIR)/etc/$(TARGET)-$(SERVICE).desktop $(REAL_USER_HOME)/.local/share/applications/
endif
	@echo -e '\033[1;35mUse XFCE4-terminal to add launchers to panel by following hyperlinks:\033[m'
	@printf -- ' \033[32mfile://%s\033[m\n' $(REAL_USER_HOME)/.local/share/applications/$(TARGET)-*.desktop

_remove:
	$(info Removing $(SERVICE)...)
ifeq ($(SERVICE), all)
	$(RM_CMD) /usr/local/bin/$(TARGET)
	$(RM_CMD) /usr/share/bash-completion/completions/$(TARGET)
	$(RM_CMD) $(REAL_USER_HOME)/.local/share/applications/$(TARGET)-*.desktop
	$(RM_CMD) $(REAL_USER_HOME)/.local/share/icons/$(TARGET)-*.svg
	$(RM_CMD) $(REAL_USER_HOME)/.config/autostart/$(TARGET).desktop
else
	$(RM_CMD) $(REAL_USER_HOME)/.local/share/applications/$(TARGET)-$(SERVICE).desktop
	$(RM_CMD) $(REAL_USER_HOME)/.local/share/icons/$(TARGET)-$(SERVICE)*.svg
endif

_clean:
	$(info Cleaning object files...)
	rm -f $(OBJS)

# The `toggled` binary should be installed in the system PATH /usr/local/bin and not in
# the user-specific PATH ~/.local/bin to ensure proper functionality of the launchers
_check_permission:
	@if [ ! -w /usr/local/bin ]; then \
		printf "\033[1;31mPermission denied\033[m\n"; \
		exit 4; \
	fi

# Prompt user to get SERVICE name if it is not already specified as an argument to `make`.
_get_input_and_run:
ifeq ($(SERVICE),)
	@read -ep $(INPUT_PS) input; \
	if [ -n "$$input" ]; then \
		$(MAKE) --no-print-directory _$(filter install remove,$(MAKECMDGOALS)) \
			SERVICE="$$input" $(filter-out install remove,$(MAKECMDGOALS)); \
	else \
		echo Input cannot be empty >&2; \
		exit 5; \
	fi
else
	@$(MAKE) --no-print-directory _$(filter install remove,$(MAKECMDGOALS)) \
		$(filter-out install remove,$(MAKECMDGOALS))
endif

# Dummy target to supress warning when `y` is used
y:
	@exit 0
