#!/usr/bin/bash

set -euo pipefail

rm -rf cmake-build-debug cmake-build-release CMakeFiles coverage_report .mypy_cache .ruff_cache
rm -f cmake_install.cmake compile_commands.json CMakeCache.txt Makefile coverage.info sandbox sandbox_coverage

find . -type d -name __pycache__ -exec rm -rf {} +
rm -rf .pytest_cache
