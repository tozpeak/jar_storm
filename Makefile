.PHONY: dir clean

# Project Settings
debug ?= 0
warnings ?= 0
NAME := jar_storm
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
LIB_DIR := lib
# TESTS_DIR := tests
BIN_DIR := bin

# Generate paths for all object files
OBJS := $(patsubst %.c,%.o, $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/systems/*.c) $(wildcard $(SRC_DIR)/game_states/*.c) $(wildcard $(LIB_DIR)/**/*.c))

# Compiler settings
CC := gcc
# LINTER := clang-tidy-18
# FORMATTER := clang-format-18

# Compiler and Linker flags Settings:
# 	-std=gnu17: Use the GNU17 standard
# 	-D _GNU_SOURCE: Use GNU extensions
# 	-D __STDC_WANT_LIB_EXT1__: Use C11 extensions
# 	-Wall: Enable all warnings
# 	-Wextra: Enable extra warnings
# 	-pedantic: Enable pedantic warnings
# 	-lm: Link to libm
#   -Wl,--verbose: verbose linker
CFLAGS :=  -std=c99 -I./$(INCLUDE_DIR) -L./$(LIB_DIR)
LDFLAGS := -l:libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11

ifeq ($(debug), 1)
	CFLAGS := $(CFLAGS) -g -O0
else
	CFLAGS := $(CFLAGS) -Oz
endif

ifeq ($(warnings), 1)
	CFLAGS := $(CFLAGS) -Wall -Wextra -pedantic
else
	CFLAGS := $(CFLAGS) -Wno-unused-result
endif

# Targets

# Build executable
$(NAME): dir $(OBJS)
	$(CC) $(patsubst %, build/%, $(OBJS)) $(CFLAGS) $(LDFLAGS) -o $(BIN_DIR)/$@

# Build object files and third-party libraries
$(OBJS): dir
	@mkdir -p $(BUILD_DIR)/$(@D)
	@$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ -c $*.c

# Run CUnit tests
# test: dir
#	@$(CC) $(CFLAGS) -lcunit -o $(BIN_DIR)/$(NAME)_test $(TESTS_DIR)/*.c
#	@$(BIN_DIR)/$(NAME)_test

# Run linter on source directories
#lint:
#	@$(LINTER) --config-file=.clang-tidy $(SRC_DIR)/* $(INCLUDE_DIR)/* $(TESTS_DIR)/* -- $(CFLAGS)

# Run formatter on source directories
#format:
#	@$(FORMATTER) -style=file -i $(SRC_DIR)/* $(INCLUDE_DIR)/* $(TESTS_DIR)/*

# Run valgrind memory checker on executable
#check: $(NAME)
#	@sudo valgrind -s --leak-check=full --show-leak-kinds=all $(BIN_DIR)/$< --help
#	@sudo valgrind -s --leak-check=full --show-leak-kinds=all $(BIN_DIR)/$< --version
#	@sudo valgrind -s --leak-check=full --show-leak-kinds=all $(BIN_DIR)/$< -v

# Setup dependencies for build and development
#setup:
#	# Update apt and upgrade packages
#	@sudo apt update
#	@sudo DEBIAN_FRONTEND=noninteractive apt upgrade -y

#	# Install OS dependencies
#	@sudo apt install -y bash libarchive-tools lsb-release wget software-properties-common gnupg

#	# Install LLVM tools required for building the project
#	@wget https://apt.llvm.org/llvm.sh
#	@chmod +x llvm.sh
#	@sudo ./llvm.sh 18
#	@rm llvm.sh

#	# Install Clang development tools
#	@sudo apt install -y clang-tools-18 clang-format-18 clang-tidy-18 valgrind bear

#	# Install CUnit testing framework
#	@sudo apt install -y libcunit1 libcunit1-doc libcunit1-dev

#	# Cleanup
#	@sudo apt autoremove -y

# Setup build and bin directories
dir:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# Clean build and bin directories
clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

# Run bear to generate compile_commands.json
#bear:
#	bear --exclude $(LIB_DIR) make $(NAME)
