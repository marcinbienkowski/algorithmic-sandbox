import pytest


class Config:
    debug_mode: bool = False


tests_options = Config()


def pytest_addoption(parser: pytest.Parser) -> None:
    parser.addoption(
        '--print-debug', action='store_true', default=False,
        help='Enable debug output from sandbox',
    )


def pytest_configure(config: pytest.Config) -> None:
    tests_options.debug_mode = config.getoption('--print-debug')
