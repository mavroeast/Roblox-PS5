# Roblox-PS5 - Local Linux/WSL build entry points (derived from ps5-native-app-boilerplate).
# SPDX-License-Identifier: GPL-3.0-or-later

SHELL := /bin/bash
.DEFAULT_GOAL := app

APP_DEFINITIONS ?= ROBLOX_PS5_MILESTONE=1
APP_INCLUDE_PATHS ?= src
APP_STATIC_ARCHIVES ?=
APP_RUNTIME_MODULES ?=

PACBREW_PACKAGES ?=
PACBREW_INCLUDE_PATHS ?=
PACBREW_STATIC_ARCHIVES ?=

TITLE_ID ?= RBLX00001
APP_NAME ?= Roblox PS5
APP_CATEGORY ?= game
CONTENT_SUFFIX ?=

HOST_CXX ?= clang++
HOST_BUILD_DIR ?= build/host-tests

export APP_DEFINITIONS APP_INCLUDE_PATHS APP_STATIC_ARCHIVES APP_RUNTIME_MODULES
export PACBREW_PACKAGES PACBREW_INCLUDE_PATHS PACBREW_STATIC_ARCHIVES
export TITLE_ID APP_NAME APP_CATEGORY CONTENT_SUFFIX

RUNTIME := runtime/libc.prx
RUNTIME_INPUTS := tools/rebuild-libc.sh \
	$(wildcard tooling/native/*.cpp tooling/native/*.hpp) \
	$(wildcard tooling/native/runtime/*.txt)

.PHONY: all app build init doctor deps assets-check libc ffpkg ffpfsc packages host-test clean distclean help

all: app
build: app

init:
	@printf '%s\n' '==> [init] Configuring application identity in sce_sys/param.json'
	@bash tools/init-project.sh sce_sys/param.json

doctor:
	@printf '%s\n' '==> [doctor] Checking Linux/WSL host prerequisites'
	@bash tools/doctor.sh

deps:
	@printf '%s\n' '==> [deps] Fetching pinned PS5 payload SDK & native build dependencies'
	@bash tools/setup-native-dependencies.sh

assets-check:
	@printf '%s\n' '==> [assets] Validating sce_sys presentation assets'
	@bash tools/validate-assets.sh

libc:
	@printf '%s\n' '==> [libc] Rebuilding clean-room runtime/libc.prx'
	@bash tools/rebuild-libc.sh

$(RUNTIME): $(RUNTIME_INPUTS)
	@printf '%s\n' '==> [libc] Generating missing or outdated runtime/libc.prx'
	@bash tools/rebuild-libc.sh

app: $(RUNTIME)
	@printf '%s\n' '==> [app] Compiling, linking, signing, and assembling dist/$(TITLE_ID)/'
	@bash tools/build.sh Folder

ffpkg: $(RUNTIME)
	@bash tools/build.sh Ffpkg

ffpfsc: $(RUNTIME)
	@bash tools/build.sh Ffpfsc

packages: $(RUNTIME)
	@bash tools/build.sh All

host-test:
	@cmake -S . -B $(HOST_BUILD_DIR) -DCMAKE_CXX_COMPILER=$(HOST_CXX)
	@cmake --build $(HOST_BUILD_DIR)
	@ctest --test-dir $(HOST_BUILD_DIR) --output-on-failure

clean:
	@rm -rf -- build dist
	@rm -f -- $(RUNTIME)

distclean: clean
	@rm -rf -- .deps

help:
	@printf '%s\n' \
		'make              Build native PS5 title folder dist/<TITLE_ID>/' \
		'make ffpkg        Build standalone FFPKG package under dist/' \
		'make ffpfsc       Build FFPFSC package under dist/' \
		'make packages     Build folder + FFPKG + FFPFSC outputs under dist/' \
		'make doctor       Verify Linux/WSL prerequisites' \
		'make deps         Fetch pinned ps5-payload-dev/sdk into .deps/' \
		'make host-test    Build and run host-side PAL unit tests via CMake/CTest' \
		'make clean        Remove build/, dist/, and runtime/libc.prx'
