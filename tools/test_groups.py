import argparse
import subprocess
import sys
from pathlib import Path
from typing import Optional, Sequence


PROJECT_ROOT = Path(__file__).resolve().parent.parent
TEST_RUNNER = PROJECT_ROOT / "tools" / "test.py"
TEST_GROUPS = (
    ("ast", PROJECT_ROOT / "tests" / "ast"),
    ("run", PROJECT_ROOT / "tests" / "run"),
)


def parse_arguments(arguments: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Process all Skard golden-test groups.")
    parser.add_argument("action", choices=("test", "generate"))
    parser.add_argument("executable", help="Path or command name of the Skard executable.")
    return parser.parse_args(arguments)


def main(arguments: Optional[Sequence[str]] = None) -> int:
    args = parse_arguments(arguments)
    failed = False

    for command, tests_dir in TEST_GROUPS:
        result = subprocess.run(
            [
                sys.executable,
                str(TEST_RUNNER),
                args.action,
                args.executable,
                "--command",
                command,
                "--tests-dir",
                str(tests_dir),
                "--no-color",
            ],
            cwd=PROJECT_ROOT,
            check=False,
        )
        failed = failed or result.returncode != 0

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
