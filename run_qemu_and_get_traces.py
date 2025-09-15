#!/usr/bin/env python3
"""
ReactOS Stack Trace Resolver
Resolves stack traces from ReactOS logs by mapping module offsets to symbol names using objdump.
"""

import re
import sys
import argparse
import subprocess
import os
import glob
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
import json
import time
import signal


@dataclass
class StackFrame:
    """Represents a single stack frame"""
    address: str
    module: str
    offset: str
    symbol: Optional[str] = None
    symbol_offset: Optional[str] = None


class SymbolCache:
    """Caches symbol lookups to improve performance"""

    def __init__(self, cache_file: str = ".symbol_cache.json"):
        self.cache_file = cache_file
        self.cache: Dict[str, Dict[str, str]] = {}
        self.load_cache()

    def load_cache(self):
        """Load cache from disk if it exists"""
        try:
            if os.path.exists(self.cache_file):
                with open(self.cache_file, 'r') as f:
                    self.cache = json.load(f)
        except Exception:
            self.cache = {}

    def save_cache(self):
        """Save cache to disk"""
        try:
            with open(self.cache_file, 'w') as f:
                json.dump(self.cache, f, indent=2)
        except Exception:
            pass

    def get(self, module: str, offset: str) -> Optional[str]:
        """Get cached symbol for module:offset"""
        if module in self.cache and offset in self.cache[module]:
            return self.cache[module][offset]
        return None

    def set(self, module: str, offset: str, symbol: str):
        """Cache a symbol lookup"""
        if module not in self.cache:
            self.cache[module] = {}
        self.cache[module][offset] = symbol


class StackTraceResolver:
    """Main class for resolving stack traces"""

    # Pattern to match stack trace lines like [FFFFF88005178B80] 1<scsiport.sys:1065>
    STACK_PATTERN = re.compile(r'\[([0-9A-Fa-f]+)\]\s*\d*<([^:]+):([0-9A-Fa-f]+)>')

    def __init__(self, binary_paths: List[str], objdump_path: str = None, verbose: bool = False):
        self.binary_paths = [Path(p).resolve() for p in binary_paths]  # Use absolute paths
        self.verbose = verbose
        self.objdump_path = objdump_path or self._find_objdump()
        self.symbol_cache = SymbolCache()
        self.module_cache: Dict[str, Optional[Path]] = {}
        self.module_index = {}
        self._build_module_index()  # Pre-build index of available modules

    def _build_module_index(self):
        """Pre-build an index of available modules for faster lookup"""
        if self.verbose:
            print("Building module index...", file=sys.stderr)

        for base_path in self.binary_paths:
            if not base_path.exists():
                continue

            # Find all .sys and .exe files
            for ext in ['*.sys', '*.exe']:
                try:
                    # Use find for comprehensive search
                    find_cmd = ['find', str(base_path), '-name', ext, '-type', 'f']
                    result = subprocess.run(find_cmd, capture_output=True, text=True, timeout=10)

                    if result.returncode == 0:
                        for line in result.stdout.strip().split('\n'):
                            if line:
                                file_path = Path(line)
                                file_name = file_path.name
                                # Store the first occurrence of each module
                                if file_name not in self.module_index:
                                    self.module_index[file_name] = file_path
                                    if self.verbose:
                                        print(f"  Indexed: {file_name} -> {file_path}", file=sys.stderr)
                except Exception as e:
                    if self.verbose:
                        print(f"Error indexing {base_path}: {e}", file=sys.stderr)

        if self.verbose:
            print(f"Module index built: {len(self.module_index)} modules found", file=sys.stderr)

    def _find_objdump(self) -> str:
        """Find objdump executable, preferring x86_64 version for AMD64 binaries"""
        candidates = [
            'x86_64-w64-mingw32-objdump',
            'amd64-w64-mingw32-objdump',
            'objdump'
        ]

        for candidate in candidates:
            try:
                result = subprocess.run([candidate, '--version'],
                                     capture_output=True, text=True)
                if result.returncode == 0:
                    if self.verbose:
                        print(f"Using objdump: {candidate}", file=sys.stderr)
                    return candidate
            except FileNotFoundError:
                continue

        raise RuntimeError("Could not find objdump executable")

    def _find_module_from_index(self, module_name: str) -> Optional[Path]:
        """Try to find module from pre-built index first"""
        if module_name in self.module_index:
            return self.module_index[module_name]
        return None

    def _find_module(self, module_name: str) -> Optional[Path]:
        """Find module binary in search paths using dynamic discovery"""
        # Check cache first
        if module_name in self.module_cache:
            return self.module_cache[module_name]

        # Try the pre-built index
        indexed_path = self._find_module_from_index(module_name)
        if indexed_path:
            self.module_cache[module_name] = indexed_path
            if self.verbose:
                print(f"Found {module_name} at {indexed_path} (from index)", file=sys.stderr)
            return indexed_path

        # Common locations within ReactOS build
        static_search_dirs = [
            "reactos",
            "reactos/system32",
            "reactos/system32/drivers",
            "drivers",
            "ntoskrnl",
            "hal",
            "boot/freeldr/freeldr",
            ".",
        ]

        for base_path in self.binary_paths:
            # Direct path check
            direct_path = base_path / module_name
            if direct_path.exists():
                self.module_cache[module_name] = direct_path
                if self.verbose:
                    print(f"Found {module_name} at {direct_path}", file=sys.stderr)
                return direct_path

            # Check static subdirectories
            for subdir in static_search_dirs:
                full_path = base_path / subdir / module_name
                if full_path.exists():
                    self.module_cache[module_name] = full_path
                    if self.verbose:
                        print(f"Found {module_name} at {full_path}", file=sys.stderr)
                    return full_path

            # Dynamic search for .sys files in drivers directory structure
            if module_name.endswith('.sys'):
                # Additional pattern-based search for common driver locations
                driver_patterns = [
                    f"drivers/storage/port/*/{module_name}",
                    f"drivers/storage/ide/*/{module_name}",
                    f"drivers/storage/class/*/{module_name}",
                    f"drivers/filesystems/*/{module_name}",
                    f"drivers/network/*/{module_name}",
                    f"drivers/bus/*/{module_name}",
                    f"drivers/hid/*/{module_name}",
                    f"drivers/usb/*/{module_name}",
                    f"drivers/filters/*/{module_name}",
                    f"drivers/serial/*/{module_name}",
                    f"drivers/input/*/{module_name}",
                    f"drivers/wdm/*/{module_name}",
                    f"drivers/crypto/*/{module_name}",
                    f"drivers/ksfilter/*/{module_name}",
                    f"drivers/processor/*/{module_name}",
                    f"drivers/setup/*/{module_name}",
                ]

                for pattern in driver_patterns:
                    matches = glob.glob(str(base_path / pattern))
                    if matches:
                        found_path = Path(matches[0])
                        if found_path.exists():
                            self.module_cache[module_name] = found_path
                            if self.verbose:
                                print(f"Found {module_name} at {found_path} (via glob)", file=sys.stderr)
                            return found_path

        # Cache negative result
        self.module_cache[module_name] = None
        if self.verbose:
            print(f"Could not find {module_name} in any search path", file=sys.stderr)
        return None

    def _resolve_symbol(self, module_path: Path, offset: str) -> Tuple[Optional[str], Optional[str]]:
        """Resolve a symbol using objdump"""
        # Check cache first
        cached = self.symbol_cache.get(module_path.name, offset)
        if cached:
            parts = cached.split('+', 1)
            if len(parts) == 2:
                return parts[0], parts[1]
            return cached, None

        try:
            # Convert hex offset to decimal
            offset_int = int(offset, 16)

            # Try using nm first for better symbol resolution
            nm_cmd = self.objdump_path.replace('objdump', 'nm')
            if 'nm' in nm_cmd:
                cmd = [nm_cmd, '-n', str(module_path)]
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)

                if result.returncode == 0:
                    # Parse nm output to find nearest symbol
                    symbols = []
                    base_addr = None
                    first_text_addr = None

                    for line in result.stdout.split('\n'):
                        parts = line.split()
                        if len(parts) >= 3:
                            try:
                                # nm format: address type symbol
                                addr_str = parts[0]
                                sym_type = parts[1]
                                sym_name = parts[2]

                                # Only consider text symbols (T, t)
                                if sym_type.upper() == 'T' or sym_type.lower() == 't':
                                    sym_addr = int(addr_str, 16)

                                    # Track the first text symbol to detect base address
                                    if first_text_addr is None:
                                        first_text_addr = sym_addr
                                        # For drivers, the base is typically aligned to 0x1000
                                        # The RVA starts from 0x1000 for the text section
                                        base_addr = (sym_addr & ~0xFFF) - 0x1000

                                    # Calculate RVA
                                    if base_addr:
                                        rva = sym_addr - base_addr
                                    else:
                                        rva = sym_addr

                                    symbols.append((rva, sym_name))
                            except (ValueError, IndexError):
                                continue

                    # Sort symbols by address
                    symbols.sort(key=lambda x: x[0])

                    # Find the symbol with the largest address <= offset
                    best_symbol = None
                    best_addr = 0
                    for sym_addr, sym_name in symbols:
                        if sym_addr <= offset_int:
                            best_symbol = sym_name
                            best_addr = sym_addr
                        else:
                            break

                    if best_symbol:
                        symbol_offset = f"0x{offset_int - best_addr:x}"
                        cache_value = f"{best_symbol}+{symbol_offset}"
                        self.symbol_cache.set(module_path.name, offset, cache_value)
                        return best_symbol, symbol_offset

            # Fallback to objdump
            cmd = [self.objdump_path, '-t', str(module_path)]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)

            if result.returncode == 0:
                # Parse symbol table to find nearest symbol
                symbols = []
                for line in result.stdout.split('\n'):
                    # Match lines like: 0000000000001065 g     F .text  0000000000000100 FdoDispatchPnp
                    parts = line.split()
                    if len(parts) >= 6 and all(c in '0123456789abcdefABCDEF' for c in parts[0]):
                        try:
                            sym_addr = int(parts[0], 16)
                            sym_name = parts[-1]
                            symbols.append((sym_addr, sym_name))
                        except ValueError:
                            continue

                # Sort symbols by address
                symbols.sort(key=lambda x: x[0])

                # Find the symbol with the largest address <= offset
                best_symbol = None
                best_addr = 0
                for sym_addr, sym_name in symbols:
                    if sym_addr <= offset_int:
                        best_symbol = sym_name
                        best_addr = sym_addr
                    else:
                        break

                if best_symbol:
                    symbol_offset = f"0x{offset_int - best_addr:x}"
                    cache_value = f"{best_symbol}+{symbol_offset}"
                    self.symbol_cache.set(module_path.name, offset, cache_value)
                    return best_symbol, symbol_offset

            # Try addr2line as last fallback
            addr2line_cmd = self.objdump_path.replace('objdump', 'addr2line')
            if 'addr2line' in addr2line_cmd:
                cmd = [addr2line_cmd, '-f', '-e', str(module_path), f'0x{offset}']
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)

                if result.returncode == 0:
                    lines = result.stdout.strip().split('\n')
                    if len(lines) >= 1 and lines[0] and lines[0] != '??':
                        symbol = lines[0].strip()
                        self.symbol_cache.set(module_path.name, offset, symbol)
                        return symbol, None

        except subprocess.TimeoutExpired:
            if self.verbose:
                print(f"Timeout resolving {module_path.name}:{offset}", file=sys.stderr)
        except Exception as e:
            if self.verbose:
                print(f"Error resolving {module_path.name}:{offset}: {e}", file=sys.stderr)

        return None, None

    def parse_stack_trace(self, text: str) -> List[StackFrame]:
        """Parse stack trace from text"""
        frames = []

        for line in text.split('\n'):
            match = self.STACK_PATTERN.search(line)
            if match:
                address, module, offset = match.groups()
                frames.append(StackFrame(
                    address=address.upper(),
                    module=module,
                    offset=offset.lower()
                ))

        return frames

    def resolve_frames(self, frames: List[StackFrame]) -> List[StackFrame]:
        """Resolve symbols for stack frames"""
        resolved_frames = []

        for frame in frames:
            module_path = self._find_module(frame.module)

            if module_path:
                symbol, symbol_offset = self._resolve_symbol(module_path, frame.offset)
                frame.symbol = symbol
                frame.symbol_offset = symbol_offset
            elif self.verbose:
                print(f"Warning: Could not find module {frame.module}", file=sys.stderr)

            resolved_frames.append(frame)

        # Save cache after resolving all frames
        self.symbol_cache.save_cache()

        return resolved_frames

    def format_output(self, frames: List[StackFrame], group_traces: bool = False) -> str:
        """Format resolved stack frames for output"""
        if not frames:
            return "No stack traces found\n"

        output = []

        if group_traces:
            # Group consecutive frames as single trace
            output.append("Stack Trace:")
            for i, frame in enumerate(frames):
                line = f"  #{i:2d}  {frame.module}:0x{frame.offset}"

                if frame.symbol:
                    if frame.symbol_offset:
                        line += f" → {frame.symbol}+{frame.symbol_offset}"
                    else:
                        line += f" → {frame.symbol}"
                else:
                    line += " → <unresolved>"

                output.append(line)
        else:
            # Output each frame individually
            for frame in frames:
                line = f"[{frame.address}] {frame.module}:0x{frame.offset}"

                if frame.symbol:
                    if frame.symbol_offset:
                        line += f" → {frame.symbol}+{frame.symbol_offset}"
                    else:
                        line += f" → {frame.symbol}"

                output.append(line)

        return '\n'.join(output) + '\n'

    def process_file(self, file_path: Optional[str], group_traces: bool = False) -> str:
        """Process a file or stdin"""
        if file_path and file_path != '-':
            with open(file_path, 'r') as f:
                text = f.read()
        else:
            text = sys.stdin.read()

        frames = self.parse_stack_trace(text)
        resolved_frames = self.resolve_frames(frames)
        return self.format_output(resolved_frames, group_traces)


class QemuLauncher:
    """Launch QEMU and monitor for ReactOS debugger readiness"""

    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.qemu_process = None
        self.log_file = '/tmp/out.log'

    def cleanup(self):
        """Clean up QEMU process and resources"""
        if self.qemu_process:
            try:
                # Try graceful termination first
                self.qemu_process.terminate()
                time.sleep(1)

                # Force kill if still running
                if self.qemu_process.poll() is None:
                    self.qemu_process.kill()
                    self.qemu_process.wait(timeout=5)
            except Exception as e:
                if self.verbose:
                    print(f"Error cleaning up QEMU: {e}", file=sys.stderr)

    def launch_and_monitor(self, timeout: int = 60) -> bool:
        """
        Launch QEMU and monitor for ReactOS debugger
        Returns True if debugger prompt detected, False on timeout
        """
        # Clean up any existing log file
        if os.path.exists(self.log_file):
            os.remove(self.log_file)

        # Build QEMU command
        qemu_cmd = [
            'qemu-system-x86_64',
            '-cdrom', 'livecd.iso',
            '-serial', f'file:{self.log_file}',
            '-m', '2G'
        ]

        print("Launching QEMU with ReactOS...")
        if self.verbose:
            print(f"Command: {' '.join(qemu_cmd)}", file=sys.stderr)

        try:
            # Launch QEMU
            self.qemu_process = subprocess.Popen(
                qemu_cmd,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )

            # Monitor log file for debugger prompt
            start_time = time.time()
            last_size = 0

            print(f"Monitoring for ReactOS debugger (timeout: {timeout}s)...")

            while time.time() - start_time < timeout:
                # Check if QEMU is still running
                if self.qemu_process.poll() is not None:
                    print("QEMU process terminated unexpectedly", file=sys.stderr)
                    return False

                # Check if log file exists and has content
                if os.path.exists(self.log_file):
                    try:
                        with open(self.log_file, 'r', encoding='utf-8', errors='replace') as f:
                            content = f.read()

                            # Check for assertion failure - if found, don't capture backtrace
                            if "Assertion failed:" in content:
                                print("Assertion failed detected - stopping QEMU without backtrace capture")
                                self.cleanup()
                                return False

                            # Check for embedded debugger entry
                            if "Entered debugger on embedded" in content:
                                print("Embedded debugger detected - waiting 1 second before stopping")
                                time.sleep(1)  # Wait 1 second after detecting the message
                                self.cleanup()
                                return True

                            # Check for debugger prompt (alternative detection)
                            if "for a list of commands" in content:
                                print("ReactOS debugger detected - capturing crash state")
                                self.cleanup()
                                return True

                            # Show progress if file is growing
                            current_size = len(content)
                            if current_size > last_size and self.verbose:
                                print(f"  Log size: {current_size} bytes", file=sys.stderr)
                                last_size = current_size
                    except Exception as e:
                        if self.verbose:
                            print(f"Error reading log: {e}", file=sys.stderr)

                # Wait before next check
                time.sleep(0.5)

            print(f"Timeout reached after {timeout} seconds - no debugger prompt detected")
            self.cleanup()
            return False

        except KeyboardInterrupt:
            print("\nInterrupted by user")
            self.cleanup()
            raise
        except Exception as e:
            print(f"Error during QEMU launch: {e}", file=sys.stderr)
            self.cleanup()
            return False

    def get_log_tail(self, lines: int = 200) -> str:
        """Get the last N lines from the log file"""
        try:
            result = subprocess.run(
                ['tail', '-n', str(lines), self.log_file],
                capture_output=True,
                text=True,
                timeout=5
            )
            if result.returncode == 0:
                return result.stdout
            else:
                # Fallback to Python if tail command fails
                with open(self.log_file, 'r', encoding='utf-8', errors='replace') as f:
                    all_lines = f.readlines()
                    return ''.join(all_lines[-lines:])
        except Exception as e:
            print(f"Error getting log tail: {e}", file=sys.stderr)
            return ""


def main():
    parser = argparse.ArgumentParser(
        description='Resolve ReactOS stack traces using objdump',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Auto-launch QEMU and capture crash (no arguments)
  %(prog)s

  # Resolve stack traces from a log file
  %(prog)s qemu.log -p output-MinGW-amd64

  # Resolve from stdin
  cat /tmp/qemu.log | %(prog)s -p output-MinGW-amd64

  # Group consecutive frames as single trace
  %(prog)s qemu.log -p output-MinGW-amd64 -g

  # Verbose output with multiple search paths
  %(prog)s qemu.log -p build/amd64 -p build/drivers -v

  # Auto-launch with custom timeout
  %(prog)s --timeout 120
        """
    )

    parser.add_argument('input_file', nargs='?', default=None,
                      help='Input file containing stack traces (if not provided, launches QEMU)')

    parser.add_argument('-p', '--path', action='append', dest='binary_paths',
                      help='Path to search for binaries (can be specified multiple times)')

    parser.add_argument('-o', '--objdump', help='Path to objdump executable')

    parser.add_argument('-g', '--group', action='store_true',
                      help='Group consecutive frames as single stack trace')

    parser.add_argument('-v', '--verbose', action='store_true',
                      help='Enable verbose output')

    parser.add_argument('--clear-cache', action='store_true',
                      help='Clear symbol cache before processing')

    parser.add_argument('--timeout', type=int, default=60,
                      help='Timeout in seconds for QEMU monitoring (default: 60)')

    args = parser.parse_args()

    # Default search paths if none specified
    if not args.binary_paths:
        args.binary_paths = ['.', 'output-MinGW-amd64', 'reactos']

    # Clear cache if requested
    if args.clear_cache:
        cache_file = '.symbol_cache.json'
        if os.path.exists(cache_file):
            os.remove(cache_file)
            if args.verbose:
                print(f"Cleared symbol cache", file=sys.stderr)

    try:
        # Check if we should launch QEMU (no input file provided)
        if args.input_file is None:
            # Auto-launch mode
            launcher = QemuLauncher(verbose=args.verbose)

            # Set up signal handler for cleanup
            def signal_handler(sig, frame):
                launcher.cleanup()
                sys.exit(0)

            signal.signal(signal.SIGINT, signal_handler)
            signal.signal(signal.SIGTERM, signal_handler)

            # Launch QEMU and monitor
            success = launcher.launch_and_monitor(timeout=args.timeout)

            # Check if assertion failure was detected
            if os.path.exists('/tmp/out.log'):
                with open('/tmp/out.log', 'r', encoding='utf-8', errors='replace') as f:
                    log_content = f.read()
                    if "Assertion failed:" in log_content:
                        # Just print the log for assertion failures
                        print("\n" + "="*70)
                        print("Assertion failure detected - showing log output:")
                        print("="*70)
                        tail_output = launcher.get_log_tail(200)
                        print(tail_output)
                        print("="*70)
                        sys.exit(0)

            if success:
                # Show last 200 lines of log
                print("\n" + "="*70)
                print("Last 200 lines of ReactOS output:")
                print("="*70)
                tail_output = launcher.get_log_tail(200)
                print(tail_output)

                # Now parse and resolve stack traces from the log
                print("\n" + "="*70)
                print("Resolved Stack Trace:")
                print("="*70)

                resolver = StackTraceResolver(
                    binary_paths=args.binary_paths,
                    objdump_path=args.objdump,
                    verbose=args.verbose
                )

                # Read the entire log for stack trace parsing
                with open('/tmp/out.log', 'r', encoding='utf-8', errors='replace') as f:
                    log_content = f.read()

                frames = resolver.parse_stack_trace(log_content)
                resolved_frames = resolver.resolve_frames(frames)
                output = resolver.format_output(resolved_frames, args.group)
                print(output)

                print("="*70)
                print("Crash has been captured and analyzed successfully.")
                print("="*70)
            else:
                print("\nNo crash detected within timeout period.")
                sys.exit(1)
        else:
            # Normal file processing mode
            resolver = StackTraceResolver(
                binary_paths=args.binary_paths,
                objdump_path=args.objdump,
                verbose=args.verbose
            )

            # Handle stdin input
            if args.input_file == '-':
                output = resolver.process_file(None, args.group)
            else:
                output = resolver.process_file(args.input_file, args.group)
            print(output, end='')

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()