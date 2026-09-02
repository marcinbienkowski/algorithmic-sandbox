#!/bin/bash

set -euo pipefail

# Uses root build, same tree that CI/README/tests use

echo "Cleaning up old coverage files..."
rm -f coverage.info
rm -rf coverage_report

echo "Building coverage-enabled executable..."
cmake .
cmake --build . --target clean-build
cmake .
cmake --build . --target sandbox_coverage
cmake --build . --target sandbox

echo "Running tests..."
uv run pytest -v tests
# For single test coverage replace the line above by, e.g.,
# uv run pytest -v tests/test_sandbox.py::test_prog_hello_world

echo "Generating coverage report..."
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_report

echo "Coverage report generated successfully!"
echo "Open coverage_report/index.html in your browser to view the results."
