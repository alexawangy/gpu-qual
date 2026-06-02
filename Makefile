.PHONY: help list configure build test run install compile-commands dev strict release relwithdebinfo clean

CMAKE ?= cmake
CTEST ?= ctest

PRESET ?= dev
INSTALL_PREFIX ?= $(CURDIR)/dist

BUILD_DIR = build/$(PRESET)
BINARY = $(BUILD_DIR)/gpu-qual

help:
	@printf '%s\n' 'Usage:'
	@printf '  %-38s %s\n' 'make dev' 'Configure, build, test, and link compile_commands.json'
	@printf '  %-38s %s\n' 'make build' 'Build using PRESET=dev by default'
	@printf '  %-38s %s\n' 'make test' 'Run tests using PRESET=dev by default'
	@printf '  %-38s %s\n' 'make run' 'Build and run ./build/<preset>/gpu-qual'
	@printf '  %-38s %s\n' 'make install PRESET=relwithdebinfo' 'Install to ./dist by default'
	@printf '  %-38s %s\n' 'make release' 'Configure, build, and test the release preset'
	@printf '  %-38s %s\n' 'make strict' 'Configure, build, and test with warnings as errors'
	@printf '  %-38s %s\n' 'make list' 'List all CMake presets'
	@printf '  %-38s %s\n' 'make clean' 'Remove generated build and dist outputs'
	@printf '%s\n' ''
	@printf '%s\n' 'Variables:'
	@printf '  %-38s %s\n' 'PRESET=dev|strict|relwithdebinfo|release' 'Select preset'
	@printf '  %-38s %s\n' 'INSTALL_PREFIX=/path' 'Select install prefix'

list:
	$(CMAKE) --list-presets=all

configure:
	$(CMAKE) --preset $(PRESET)

build: configure
	$(CMAKE) --build --preset $(PRESET)

test: build
	$(CTEST) --preset $(PRESET)

run: build
	@echo
	@echo "------------- Program Output -------------"
	@echo
	@./$(BINARY)

install: build
	$(CMAKE) --install $(BUILD_DIR) --prefix "$(INSTALL_PREFIX)"

compile-commands: configure
	ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json

dev: PRESET := dev
dev: test compile-commands

strict: PRESET := strict
strict: test compile-commands

release: PRESET := release
release: test

relwithdebinfo: PRESET := relwithdebinfo
relwithdebinfo: test

clean:
	rm -rf build dist compile_commands.json
