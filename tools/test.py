import os
import subprocess
import sys

import colorama

def get_test_files():
    test_files = []
    try:
        for root, dirs, files in os.walk("tests"):
            for file in files:
                name, extension = os.path.splitext(file)
                if extension != ".sk":
                    continue
                file_path = os.path.join(root, file)
                test_files.append(file_path)
        return test_files
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)

def run(file):
    result = subprocess.run([sys.argv[2], "ast", file], capture_output=True, text=True)
    return result

def generate(file, result):
    print(f"Generating from {file}")
    if result.returncode != 0:
        print(f"Generating from {file} finished with exit code {result.returncode}. Skipping.")
        return
    try:
        with open(file + ".expect", "w") as generated:
            generated.write(result.stdout)
    except Exception as e:
        print(f"Generating from {file} failed because {e}")

def test(file, result):
    print(f"Testing {file}")
    if result.returncode != 0:
        print(f"{colorama.Fore.RED}[Failed]{colorama.Style.RESET_ALL} Testing {file} finished with exit code {result.returncode}.")
    try:
        with open(file + ".expect", "r") as test:
            read_test = test.read()
            if result.stdout != read_test:
                print(f"{colorama.Fore.RED}[Failed]{colorama.Style.RESET_ALL} Testing {file} finished with unexpected output.")
                print("Expected:")
                print(read_test)
                print("Got:")
                print(result.stdout)
                return
            print(f"{colorama.Fore.GREEN}[Ok]{colorama.Style.RESET_ALL} Testing {file} finished.")
    except FileNotFoundError:
        print(f"{colorama.Fore.RED}[Failed]{colorama.Style.RESET_ALL} Testing {file} finished. Expect file {file + ".expect"} doesn't exist.")
    except Exception as e:
        print(f"{colorama.Fore.RED}[Failed]{colorama.Style.RESET_ALL} Testing {file} finished. Unexpected error {e}.")

def process(file):
    result = run(file)
    if sys.argv[1] == "generate":
        generate(file, result)
        return
    if sys.argv[1] == "test":
        test(file, result)
        return

if __name__ == "__main__":
    colorama.init()

    if len(sys.argv) != 3:
        print(f"Error: Wrong usage", file=sys.stderr)
        exit(1)
    test_files = get_test_files()
    for file in test_files:
        process(file)
