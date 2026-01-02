#!/usr/bin/env python3

import os
import sys
import subprocess
import platform
import argparse
import time
import fnmatch
import xml.etree.ElementTree as ET
from pathlib import Path

def generate_xunit_report(results, output_file, total_time):
	testsuites = ET.Element("testsuites")
	testsuite = ET.SubElement(testsuites, "testsuite", name="EventRecorderTest", tests=str(len(results)), time=str(total_time))

	failures = 0
	for result in results:
		# XUnit format: classname="TestSuite", name="TestCase"
		testcase = ET.SubElement(testsuite, "testcase", name=result['test_case'], classname="EventRecorderTest", time=str(result['duration']))
		if not result['success']:
			failures += 1
			failure = ET.SubElement(testcase, "failure", message=result['message'])
			failure.text = result.get('output', '')

	testsuite.set("failures", str(failures))

	tree = ET.ElementTree(testsuites)
	try:
		tree.write(output_file, encoding="utf-8", xml_declaration=True)
		# print(f"XUnit report generated at: {output_file}")
	except Exception as e:
		print(f"Error writing XUnit report: {e}")

def main():
	parser = argparse.ArgumentParser(description="Run ScummVM Event Recorder tests.")
	parser.add_argument("--xunit-output", help="Path to generate XUnit XML report", default=None)
	parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose output", default=False)
	parser.add_argument("--filter", help="Filter tests (glob pattern, e.g. *monkey*)", default="*")
	args = parser.parse_args()

	# Configuration
	# Determine default binary name based on OS
	bin_name = "scummvm.exe" if platform.system() == "Windows" else "scummvm"

	# Allow overriding via env vars
	# Default to looking in current directory
	scummvm_bin = os.getenv("SCUMMVM_BIN", bin_name)
	demos_dir = Path(os.getenv("DEMOS_DIR", "test/demos"))
	records_dir = Path(os.getenv("RECORDS_DIR", "test/records"))

	# Check if ScummVM exists
	if not Path(scummvm_bin).exists():
		print(f"Error: ScummVM binary not found at {scummvm_bin}")
		print("Please run this script from the root of the ScummVM source tree where the binary is built.")
		sys.exit(127)

	# Check if demos directory exists
	if not demos_dir.exists():
		print(f"Error: Demos directory not found at {demos_dir}")
		sys.exit(127)

	# Collect tests
	test_cases = []
	for demo_path in sorted(demos_dir.iterdir()):
		if not demo_path.is_dir():
			continue

		game_id = demo_path.name
		# Search for records matching game_id*.r??
		record_files = sorted(list(records_dir.glob(f"{game_id}*.r??")))

		for record_file in record_files:
			# Test name: EventRecorderTest.<RecordFileName>
			# We use the filename as the test case name
			test_case_name = record_file.name
			full_test_name = f"EventRecorderTest.{test_case_name}"

			# Check filter
			# Match against full name or just the test case name
			if not fnmatch.fnmatch(full_test_name, args.filter) and not fnmatch.fnmatch(test_case_name, args.filter):
				continue

			test_cases.append({
				'full_name': full_test_name,
				'test_case': test_case_name,
				'demo_path': demo_path,
				'record_file': record_file,
				'game_id': game_id
			})

	total_tests = len(test_cases)

	# Googletest compatible header
	print(f"[==========] {total_tests} tests from 1 test suite ran.")
	print(f"[----------] {total_tests} tests from EventRecorderTest")

	start_total_time = time.time()
	passed_tests = []
	failed_tests = []
	results_for_xunit = []

	for test in test_cases:
		print(f"[ RUN      ] {test['full_name']}")
		sys.stdout.flush()

		test_start_time = time.time()
		success = False
		message = ""
		output = ""

		try:
			playback_cmd = [
				str(scummvm_bin),
				"--record-mode=playback",
				f"--record-file-name={test['record_file']}",
				#"--disable-display", ## TODO: crashes with SIGFPE
				f"--path={test['demo_path']}",
				test['game_id']
			]

			# Run and capture output
			if args.verbose:
				print(f"Executing: {' '.join(playback_cmd)}")
			playback_result = subprocess.run(playback_cmd, capture_output=True, text=True)
			output = playback_result.stdout + "\n" + playback_result.stderr

			if args.verbose:
				if playback_result.stdout.strip():
					print(playback_result.stdout)
				if playback_result.stderr.strip():
					print(playback_result.stderr)

			if playback_result.returncode == 0:
				success = True
			else:
				success = False
				message = f"Exit Code: {playback_result.returncode}"

		except Exception as e:
			success = False
			message = str(e)
			output += f"\nException: {e}"

		duration_sec = time.time() - test_start_time
		duration_ms = int(duration_sec * 1000)

		results_for_xunit.append({
			'name': test['full_name'],
			'test_case': test['test_case'],
			'success': success,
			'duration': duration_sec,
			'message': message,
			'output': output
		})

		if success:
			print(f"[       OK ] {test['full_name']} ({duration_ms} ms)")
			passed_tests.append(test['full_name'])
		else:
			print(f"[  FAILED  ] {test['full_name']} ({duration_ms} ms)")
			failed_tests.append(test['full_name'])

	total_duration_sec = time.time() - start_total_time
	total_duration_ms = int(total_duration_sec * 1000)

	print(f"[----------] {total_tests} tests from EventRecorderTest ({total_duration_ms} ms total)")
	print(f"[==========] {total_tests} tests from 1 test suite ran. ({total_duration_ms} ms total)")
	print(f"[  PASSED  ] {len(passed_tests)} tests.")

	if args.xunit_output:
		generate_xunit_report(results_for_xunit, args.xunit_output, total_duration_sec)

	if failed_tests:
		print(f"[  FAILED  ] {len(failed_tests)} tests, listed below:")
		for failed in failed_tests:
			print(f"[  FAILED  ] {failed}")
		sys.exit(1)

	if total_tests == 0:
		print("No tests were run.")
		sys.exit(0)

	sys.exit(0)

if __name__ == "__main__":
	main()
