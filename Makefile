# AI-assisted scaffold.
# Prompt: Create a portable GNU Make build for modular C17 sources.

CC ?= cc
TARGET := ifj26compiler
BUILD_DIR := build

CPPFLAGS := -Isrc -Isrc/shared -Isrc/lexicalAnalyzer -Isrc/syntaxAnalyzer \
            -Isrc/abstractSyntaxTree -Isrc/semanticAnalyzer -Isrc/symbolTable \
CFLAGS ?= -std=c17 -O2
CFLAGS += -Wall -Wextra -Wpedantic
LDFLAGS ?=
LDLIBS ?=
VSCODE_CLANG_FORMAT := $(lastword $(sort $(wildcard $(HOME)/.vscode/extensions/ms-vscode.cpptools-*/LLVM/bin/clang-format)))
CLANG_FORMAT ?= $(or $(shell command -v clang-format 2>/dev/null),$(VSCODE_CLANG_FORMAT),clang-format)

SOURCES := $(wildcard src/*.c src/*/*.c)
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))
TEST_SOURCES := $(wildcard tests/unit/*.c)
TEST_BINARIES := $(patsubst tests/unit/%.c,$(BUILD_DIR)/tests/%,$(TEST_SOURCES))
PARSER_TEST_OBJECTS := $(filter-out $(BUILD_DIR)/src/main.o \
	$(BUILD_DIR)/src/lexicalAnalyzer/lexicalAnalyzer.o,$(OBJECTS))

.PHONY: all build build-check check test parser-test pipeline-driver format format-check format-test clean

all: build

build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/tests/%: tests/unit/%.c $(filter-out $(BUILD_DIR)/src/main.o,$(OBJECTS))
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/tests/parserTest: tests/unit/parserTest.c $(PARSER_TEST_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

test: $(TEST_BINARIES)
	@set -e; for testBinary in $(TEST_BINARIES); do "$$testBinary"; done

parser-test: $(BUILD_DIR)/tests/parserTest
	@$(BUILD_DIR)/tests/parserTest

$(BUILD_DIR)/tests/compilerPipelineDriver: tests/compilerPipelineDriver.c $(filter-out $(BUILD_DIR)/src/main.o,$(OBJECTS))
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

pipeline-driver: $(BUILD_DIR)/tests/compilerPipelineDriver

format:
	"$(CLANG_FORMAT)" -i $(SOURCES) $(wildcard src/*.h src/*/*.h) $(TEST_SOURCES) $(wildcard tests/*.c)

format-check:
	"$(CLANG_FORMAT)" --dry-run --Werror $(SOURCES) $(wildcard src/*.h src/*/*.h) $(TEST_SOURCES) $(wildcard tests/*.c)

format-test:
	python3 tests/formatStyleTest.py --formatter "$(CLANG_FORMAT)"

check:
	@if command -v "$(CLANG_FORMAT)" >/dev/null 2>&1; then \
		"$(CLANG_FORMAT)" --dry-run --Werror $(SOURCES) $(wildcard src/*/*.h) $(TEST_SOURCES); \
	else \
		echo "clang-format unavailable; skipped"; \
	fi

build-check: check build test

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(OBJECTS:.o=.d)
