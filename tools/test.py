import argparse
import difflib
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Optional, Sequence

import colorama


PROJECT_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_TESTS_DIR = PROJECT_ROOT / "tests"
DEFAULT_TIMEOUT_SECONDS = 5.0


def get_test_files(tests_dir: Path) -> List[Path]:
    return sorted(tests_dir.rglob("*.sk"), key=lambda path: path.as_posix())


def expectation_file(test_file: Path) -> Path:
    return test_file.with_name(test_file.name + ".expect")


def stderr_file(test_file: Path) -> Path:
    return test_file.with_name(test_file.name + ".stderr")


def exit_file(test_file: Path) -> Path:
    return test_file.with_name(test_file.name + ".exit")


def display_path(path: Path) -> str:
    try:
        return str(path.relative_to(PROJECT_ROOT))
    except ValueError:
        return str(path)


def print_status(label: str, color: str, message: str) -> None:
    print(f"{color}[{label}]{colorama.Style.RESET_ALL} {message}")


def run_test_program(
    executable: str,
    command: str,
    test_file: Path,
    timeout: float,
) -> Optional[subprocess.CompletedProcess]:
    try:
        return subprocess.run(
            [executable, command, str(test_file)],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout,
            check=False,
        )
    except subprocess.TimeoutExpired as error:
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"{display_path(test_file)} exceeded the {timeout:g} second timeout.",
        )
        if error.stdout:
            print("Partial standard output:")
            print(error.stdout)
        if error.stderr:
            print("Partial standard error:")
            print(error.stderr)
    except OSError as error:
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"Could not execute '{executable}': {error}",
        )

    return None


def generate_test(
    test_file: Path,
    executable: str,
    command: str,
    timeout: float,
) -> bool:
    path = display_path(test_file)
    print(f"Generating {path}")
    result = run_test_program(executable, command, test_file, timeout)
    if result is None:
        return False

    try:
        with expectation_file(test_file).open("w", encoding="utf-8", newline="") as generated:
            generated.write(result.stdout)

        expected_stderr_file = stderr_file(test_file)
        if result.stderr:
            with expected_stderr_file.open("w", encoding="utf-8", newline="") as generated:
                generated.write(result.stderr)
        else:
            expected_stderr_file.unlink(missing_ok=True)

        expected_exit_file = exit_file(test_file)
        if result.returncode != 0:
            with expected_exit_file.open("w", encoding="utf-8", newline="") as generated:
                generated.write(f"{result.returncode}\n")
        else:
            expected_exit_file.unlink(missing_ok=True)
    except OSError as error:
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"Could not update expectations for {path}: {error}",
        )
        return False

    print_status("Generated", colorama.Fore.GREEN, path)
    return True


def print_output_diff(expected: str, actual: str, expected_file: Path) -> None:
    diff = difflib.unified_diff(
        expected.splitlines(keepends=True),
        actual.splitlines(keepends=True),
        fromfile=display_path(expected_file),
        tofile="actual output",
    )
    rendered = "".join(diff)
    if not rendered:
        rendered = "Outputs differ in line-ending or final-newline details.\n"
    print(rendered, end="" if rendered.endswith("\n") else "\n")


def read_expected_output(expected_file: Path, default: Optional[str] = None) -> Optional[str]:
    try:
        return expected_file.read_text(encoding="utf-8")
    except FileNotFoundError:
        if default is not None:
            return default
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"Expectation file {display_path(expected_file)} does not exist.",
        )
    except OSError as error:
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"Could not read {display_path(expected_file)}: {error}",
        )

    return None


def read_expected_exit_code(expected_file: Path) -> Optional[int]:
    try:
        contents = expected_file.read_text(encoding="utf-8")
    except FileNotFoundError:
        return 0
    except OSError as error:
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"Could not read {display_path(expected_file)}: {error}",
        )
        return None

    stripped = contents.strip()
    if re.fullmatch(r"[+-]?\d+", stripped) is None:
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"Expected one integer in {display_path(expected_file)}.",
        )
        return None

    return int(stripped)


def test_file(
    source_file: Path,
    executable: str,
    command: str,
    timeout: float,
) -> bool:
    path = display_path(source_file)
    print(f"Testing {path}")
    result = run_test_program(executable, command, source_file, timeout)
    if result is None:
        return False

    expected_stdout_file = expectation_file(source_file)
    expected_stderr_file = stderr_file(source_file)
    expected_exit_file = exit_file(source_file)
    expected_stdout = read_expected_output(expected_stdout_file)
    expected_stderr = read_expected_output(expected_stderr_file, default="")
    expected_exit = read_expected_exit_code(expected_exit_file)
    if expected_stdout is None or expected_stderr is None or expected_exit is None:
        return False

    passed = True
    if result.returncode != expected_exit:
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"{path} finished with exit code {result.returncode}; expected {expected_exit}.",
        )
        passed = False

    if result.stdout != expected_stdout:
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"{path} produced unexpected standard output.",
        )
        print_output_diff(expected_stdout, result.stdout, expected_stdout_file)
        passed = False

    if result.stderr != expected_stderr:
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"{path} produced unexpected standard error.",
        )
        print_output_diff(expected_stderr, result.stderr, expected_stderr_file)
        passed = False

    if not passed:
        return False

    print_status("Ok", colorama.Fore.GREEN, path)
    return True


def positive_float(value: str) -> float:
    parsed = float(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("must be greater than zero")
    return parsed


def parse_arguments(arguments: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run or regenerate Skard golden-file tests.",
    )
    parser.add_argument("action", choices=("test", "generate"))
    parser.add_argument("executable", help="Path or command name of the Skard executable.")
    parser.add_argument(
        "--command",
        choices=("ast", "run"),
        default="ast",
        help="Skard command used for each test (default: ast).",
    )
    parser.add_argument(
        "--tests-dir",
        type=Path,
        default=DEFAULT_TESTS_DIR,
        help="Directory searched recursively for .sk files (default: project tests directory).",
    )
    parser.add_argument(
        "--timeout",
        type=positive_float,
        default=DEFAULT_TIMEOUT_SECONDS,
        help=f"Per-test timeout in seconds (default: {DEFAULT_TIMEOUT_SECONDS:g}).",
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable colored status output.",
    )
    return parser.parse_args(arguments)


def resolve_executable(value: str) -> Optional[str]:
    candidate = Path(value).expanduser()
    if candidate.is_file():
        return str(candidate.resolve())

    resolved = shutil.which(value)
    if resolved is not None:
        return resolved

    return None


def main(arguments: Optional[Sequence[str]] = None) -> int:
    args = parse_arguments(arguments)
    colorama.init(strip=True if args.no_color else None)

    executable = resolve_executable(args.executable)
    if executable is None:
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"Executable '{args.executable}' does not exist and was not found on PATH.",
        )
        return 1

    tests_dir = args.tests_dir.expanduser().resolve()
    if not tests_dir.is_dir():
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"Test directory '{tests_dir}' does not exist or is not a directory.",
        )
        return 1

    test_files = get_test_files(tests_dir)
    if not test_files:
        print_status(
            "Failed",
            colorama.Fore.RED,
            f"No .sk test files found in '{tests_dir}'.",
        )
        return 1

    operation = generate_test if args.action == "generate" else test_file
    results = [
        operation(test, executable, args.command, args.timeout)
        for test in test_files
    ]

    succeeded = results.count(True)
    failed = results.count(False)
    label = "generated" if args.action == "generate" else "passed"
    print(f"\n{succeeded} {label}, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
