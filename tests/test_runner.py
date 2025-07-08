#!/usr/bin/env python3
"""
Simple test runner for umlang runtime implementations.
Usage: ./test_runner.py <runtime_name> [test_suite]
"""

import json
import os
import platform
import subprocess
import sys
import time
from typing import Dict, List, Optional, Tuple


class TestRunner:
    def __init__(self, repo_root: str):
        self.repo_root = repo_root
        self.tests_dir = os.path.join(repo_root, "tests")
        self.umasm_dir = os.path.join(repo_root, "umasm", "umbinary")
        
        # Detect platform
        self.platform_info = self._detect_platform()
        
        # Load configurations
        with open(os.path.join(self.tests_dir, "runtimes.json")) as f:
            self.runtimes = json.load(f)
        
        with open(os.path.join(self.tests_dir, "test_cases.json")) as f:
            self.test_cases = json.load(f)
    
    def _detect_platform(self) -> Dict[str, str]:
        """Detect the current platform and return platform info."""
        system = platform.system()
        machine = platform.machine()
        
        # Normalize machine architecture names
        if machine in ['x86_64', 'AMD64']:
            arch = 'x86_64'
        elif machine in ['arm64', 'aarch64']:
            arch = 'arm64'
        else:
            arch = machine
        
        return {
            'system': system,
            'arch': arch,
            'platform_key': f"{system.lower()}-{arch}"
        }
    
    def get_compatible_runtimes(self) -> List[str]:
        """Get list of runtime names compatible with current platform."""
        compatible = []
        platform_key = self.platform_info['platform_key']
        
        for runtime_name, config in self.runtimes.items():
            # Check if runtime has platform compatibility info
            if 'platforms' in config:
                if platform_key in config['platforms'] or 'all' in config['platforms']:
                    compatible.append(runtime_name)
            else:
                # Legacy compatibility: guess from runtime name
                if platform_key == 'darwin-arm64' and 'darwin' in runtime_name:
                    compatible.append(runtime_name)
                elif platform_key == 'linux-x86_64' and 'linux-x86' in runtime_name:
                    compatible.append(runtime_name)
                elif platform_key == 'linux-arm64' and 'linux-arm64' in runtime_name:
                    compatible.append(runtime_name)
                elif runtime_name in ['interp', 'llvm-ir']:  # These should work everywhere
                    compatible.append(runtime_name)
        
        return compatible
    
    def build_runtime(self, runtime_name: str) -> bool:
        """Build the specified runtime if needed."""
        if runtime_name not in self.runtimes:
            print(f"❌ Unknown runtime: {runtime_name}")
            self._show_platform_suggestions()
            return False
        
        # Check platform compatibility
        compatible_runtimes = self.get_compatible_runtimes()
        if runtime_name not in compatible_runtimes:
            print(f"⚠️  Runtime '{runtime_name}' may not be compatible with your platform ({self.platform_info['platform_key']})")
            print("Compatible runtimes for your platform:")
            for runtime in compatible_runtimes:
                print(f"  - {runtime}")
            print("\nContinuing anyway... (build may fail)")
            print()
        
        runtime_config = self.runtimes[runtime_name]
        build_cmd = runtime_config.get("build_cmd")
        
        if not build_cmd:
            return True  # No build command needed
        
        print(f"🔨 Building {runtime_name}...")
        try:
            result = subprocess.run(
                build_cmd,
                shell=True,
                cwd=self.repo_root,
                capture_output=True,
                text=True,
                timeout=120
            )
            
            if result.returncode != 0:
                print(f"❌ Build failed for {runtime_name}:")
                print(result.stderr)
                return False
            
            print(f"✅ Build successful for {runtime_name}")
            return True
            
        except subprocess.TimeoutExpired:
            print(f"❌ Build timeout for {runtime_name}")
            return False
    
    def run_test(self, runtime_name: str, test: Dict) -> Tuple[bool, str, float]:
        """Run a single test case."""
        runtime_config = self.runtimes[runtime_name]
        executable = os.path.join(self.repo_root, runtime_config["path"])
        program_path = os.path.join(self.umasm_dir, test["program"])
        
        # Check if executable exists
        if not os.path.exists(executable):
            return False, f"Executable not found: {executable}", 0.0
        
        # Check if program exists
        if not os.path.exists(program_path):
            return False, f"Test program not found: {program_path}", 0.0
        
        # Prepare command
        cmd = [executable, program_path]
        input_data = test.get("input")
        timeout = test.get("timeout", 30)
        
        # Run the test
        start_time = time.time()
        try:
            result = subprocess.run(
                cmd,
                input=input_data,
                text=True,
                capture_output=True,
                timeout=timeout
            )
            execution_time = time.time() - start_time
            
            if result.returncode != 0:
                return False, f"Runtime error (exit code {result.returncode}): {result.stderr}", execution_time
            
            # Check output if expected is provided
            if "expected" in test:
                expected = test["expected"]
                actual = result.stdout
                
                if actual != expected:
                    return False, f"Output mismatch. Expected: {repr(expected)}, Got: {repr(actual)}", execution_time
            
            return True, result.stdout, execution_time
            
        except subprocess.TimeoutExpired:
            execution_time = time.time() - start_time
            return False, f"Timeout after {timeout}s", execution_time
        except Exception as e:
            execution_time = time.time() - start_time
            return False, f"Execution error: {str(e)}", execution_time
    
    def run_test_suite(self, runtime_name: str, suite_name: Optional[str] = None) -> None:
        """Run tests for a specific runtime."""
        if not self.build_runtime(runtime_name):
            return
        
        print(f"\n🚀 Running tests for {runtime_name}")
        print("=" * 50)
        
        # Determine which test suites to run
        if suite_name:
            if suite_name not in self.test_cases:
                print(f"❌ Unknown test suite: {suite_name}")
                return
            suites_to_run = {suite_name: self.test_cases[suite_name]}
        else:
            suites_to_run = self.test_cases
        
        total_tests = 0
        passed_tests = 0
        total_time = 0.0
        
        for suite_name, tests in suites_to_run.items():
            print(f"\n📋 {suite_name.upper()} TESTS:")
            
            for test in tests:
                total_tests += 1
                test_name = test["name"]
                
                success, message, exec_time = self.run_test(runtime_name, test)
                total_time += exec_time
                
                if success:
                    passed_tests += 1
                    if test.get("benchmark", False):
                        print(f"  ✅ {test_name} ({exec_time:.3f}s)")
                    else:
                        print(f"  ✅ {test_name}")
                else:
                    print(f"  ❌ {test_name}: {message}")
        
        # Summary
        print(f"\n📊 SUMMARY:")
        print(f"  Runtime: {runtime_name}")
        print(f"  Tests: {passed_tests}/{total_tests} passed")
        print(f"  Success Rate: {(passed_tests/total_tests)*100:.1f}%")
        print(f"  Total Time: {total_time:.3f}s")
        
        if passed_tests == total_tests:
            print("  🎉 All tests passed!")
        else:
            print(f"  ⚠️  {total_tests - passed_tests} tests failed")


    def _show_platform_suggestions(self):
        """Show platform-specific runtime suggestions."""
        print(f"\nDetected platform: {self.platform_info['system']} {self.platform_info['arch']}")
        print("Compatible runtimes for your platform:")
        compatible = self.get_compatible_runtimes()
        for runtime in compatible:
            print(f"  - {runtime}")
        print()


def main():
    if len(sys.argv) < 2:
        print("Usage: ./test_runner.py <runtime_name> [test_suite]")
        
        # Try to load runtimes to show available options
        try:
            repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
            runner = TestRunner(repo_root)
            
            print(f"\nDetected platform: {runner.platform_info['system']} {runner.platform_info['arch']}")
            
            print("\nRecommended runtimes for your platform:")
            compatible = runner.get_compatible_runtimes()
            for runtime_name in compatible:
                print(f"  - {runtime_name}")
            
            print("\nAll available runtimes:")
            for runtime_name in runner.runtimes.keys():
                marker = "✓" if runtime_name in compatible else "⚠"
                print(f"  {marker} {runtime_name}")
                
            print("\nAvailable test suites:")
            for suite_name in runner.test_cases.keys():
                print(f"  - {suite_name}")
                
        except Exception as e:
            print(f"Error loading configuration: {e}")
        
        sys.exit(1)
    
    runtime_name = sys.argv[1]
    test_suite = sys.argv[2] if len(sys.argv) > 2 else None
    
    # Find repo root
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    
    # Create and run test runner
    runner = TestRunner(repo_root)
    runner.run_test_suite(runtime_name, test_suite)


if __name__ == "__main__":
    main()