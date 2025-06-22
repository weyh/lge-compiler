#! /usr/bin/python3

import argparse
from dataclasses import dataclass
import json
import os
import subprocess
import time
from typing import List, TypedDict


class TestType(TypedDict):
    name: str
    file_name: str
    exit_code: int
    has_stdin: bool


class JsonType(TypedDict):
    version: int
    tests: List[TestType]


@dataclass
class CmdRunRet:
    exit_code: int
    stdout: str
    stderr: str


def run_command(cmd: str) -> CmdRunRet:
    try:
        result = subprocess.run(
            cmd,
            shell=True,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        return CmdRunRet(result.returncode, result.stdout, result.stderr)
    except subprocess.CalledProcessError as e:
        return CmdRunRet(e.returncode, e.stdout, e.stderr)


def main(args: argparse.Namespace):
    COMPILER_EXE: str = args.compiler
    SUITE_FILE: str = args.suite
    ARTIFACT_ROOT: str = args.artifact_root
    RUN_TIME: str = args.run_time

    EXAMPLE_ROOT = os.path.join(ARTIFACT_ROOT, 'examples')
    SNAPSHOT_ROOT = os.path.join(ARTIFACT_ROOT, 'snapshots')

    print(f"Using compiler:       {COMPILER_EXE}\n"
          f"Using suite file:     {SUITE_FILE}\n"
          f"Using artifact root:  {ARTIFACT_ROOT}\n"
          f"Example root:         {EXAMPLE_ROOT}\n"
          f"Snapshot root:        {SNAPSHOT_ROOT}")

    suite: JsonType | None = None
    with open(SUITE_FILE, 'r', encoding="utf-8") as f:
        suite = json.load(f)

    assert suite is not None
    assert os.path.exists(COMPILER_EXE)
    assert os.path.exists(RUN_TIME)

    print(f"\n📋 Test Suite v{suite['version']}")
    print("=" * 60)

    stats = {
        "total": len(suite['tests']),
        "passed": 0,
        "failed": [],
        "start_time": time.time()
    }

    for idx, test in enumerate(suite['tests'], 1):
        print(f"\n[{idx}/{stats['total']}] 🔍 Test: {test['name']}")
        paths = {
            "example": os.path.join(EXAMPLE_ROOT, f"{test['file_name']}.lge"),
            "stdin": os.path.join(SNAPSHOT_ROOT, f"{test['file_name']}_stdin.txt"),
            "stdout": os.path.join(SNAPSHOT_ROOT, f"{test['file_name']}_stdout.txt"),
            "stderr": os.path.join(SNAPSHOT_ROOT, f"{test['file_name']}_stderr.txt")
        }
        print(f"\tExample:         {paths['example']}")
        if test['has_stdin'] == True:
            print(f"\tExpected stdin:  {paths['stdin']}")
        print(f"\tExpected stdout: {paths['stdout']}")
        print(f"\tExpected stderr: {paths['stderr']}")
        print("-" * 40)

        try:
            for path_type, path in paths.items():
                if not os.path.exists(path) and not path.find("stdin"):
                    raise FileNotFoundError(
                        f"Missing {path_type} file: {path}")

            # Execute test
            if test['has_stdin'] == True:
                tmp_file = f"/tmp/tmp_{test['file_name']}.ll"
                comp_cmd = f"{COMPILER_EXE} {paths['example']} > {tmp_file}"

                print(f"⚙️  Running: '{comp_cmd}'")
                result = run_command(comp_cmd)
                if result.exit_code != 0:
                    raise AssertionError(
                        f"Exit code mismatch:\n"
                        f"  Expected: 0\n"
                        f"  Got:      {result.exit_code}"
                    )

                cmd = f"lli -load={RUN_TIME} {tmp_file} < {paths['stdin']}"
            else:
                cmd = f"{COMPILER_EXE} {paths['example']} | lli -load={RUN_TIME}"

            print(f"⚙️  Running: '{cmd}'")
            result = run_command(cmd)

            # Validate results
            if result.exit_code != test['exit_code']:
                raise AssertionError(
                    f"Exit code mismatch:\n"
                    f"  Expected: {test['exit_code']}\n"
                    f"  Got:      {result.exit_code}"
                )

            for stream_type in ['stdout', 'stderr']:
                with open(paths[stream_type], 'r', encoding="utf-8") as f:
                    expected = f.read()
                    actual = getattr(result, stream_type)
                    if expected != actual:
                        raise AssertionError(
                            f"{stream_type} mismatch:\n"
                            f"  Expected: '{expected}'\n"
                            f"  Got:      '{actual}'"
                        )

            print("✅ PASSED")
            stats['passed'] += 1

        except Exception as e:
            print(f"❌ FAILED: {str(e)}")
            stats['failed'].append((test['name'], str(e)))

    # Print summary
    duration = time.time() - stats['start_time']
    print("\n" + "=" * 60)
    print("📊 Test Summary")
    print("-" * 60)
    print(f"Total Tests:    {stats['total']}")
    print(f"Passed:         {stats['passed']}")
    print(f"Failed:         {len(stats['failed'])}")
    print(f"Duration:       {duration:.2f}s")

    if stats['failed']:
        print("\n❌ Failed Tests:")
        for name, error in stats['failed']:
            print(f"  • {name}")
            print(f"    {error}")

    success = len(stats['failed']) == 0
    print(f"\n{'🎉 All tests passed!' if success else '💔 Some tests failed.'}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        epilog="example: run.py -c ../build/lgec -r ../build/runtime/liblge_runtime.so -s suite.json -a .")

    parser.add_argument("-c", "--compiler",
                        help="Path to the built compiler to run tests on", required=True)
    parser.add_argument("-r", "--run-time",
                        help="Path to the liblge_runtime.so", required=True)
    parser.add_argument("-s", "--suite",
                        help="Suite file which contains the test definitions", required=True)
    parser.add_argument("-a", "--artifact-root", default=os.path.dirname(os.path.abspath(__file__)),
                        help="Parent folder of examples and snapshots")

    args = parser.parse_args()

    main(args)
