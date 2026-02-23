# SPDX-FileCopyrightText: 2025 Scott Shawcroft for Adafruit Industries
# SPDX-License-Identifier: MIT

"""Test LED blink functionality on native_sim."""

import pytest

from pathlib import Path
from perfetto.trace_processor import TraceProcessor


@pytest.mark.circuitpy_drive(None)
def test_blank_flash_hello_world(circuitpython):
    """Test that an erased flash shows code.py output header."""
    circuitpython.wait_until_done()

    output = circuitpython.serial.all_output
    assert "Board ID:native_native_sim" in output
    assert "UID:" in output
    assert "code.py output:" in output
    assert "Hello World" in output
    assert "done" in output


BLINK_CODE = """\
import time
import board
import digitalio

led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT

for i in range(3):
    print(f"LED on {i}")
    led.value = True
    time.sleep(0.1)
    print(f"LED off {i}")
    led.value = False
    time.sleep(0.1)

print("done")
"""


def parse_gpio_trace(trace_file: Path, pin_name: str = "gpio_emul.00") -> list[tuple[int, int]]:
    """Parse GPIO trace from Perfetto trace file."""
    tp = TraceProcessor(file_path=str(trace_file))
    result = tp.query(
        f'''
        SELECT c.ts, c.value
        FROM counter c
        JOIN track t ON c.track_id = t.id
        WHERE t.name = "{pin_name}"
        ORDER BY c.ts
        '''
    )
    return [(row.ts, int(row.value)) for row in result]


@pytest.mark.circuitpy_drive({"code.py": BLINK_CODE})
def test_blink_output(circuitpython):
    """Test blink program produces expected output and GPIO traces."""
    circuitpython.wait_until_done()

    # Check serial output
    output = circuitpython.serial.all_output
    assert "LED on 0" in output
    assert "LED off 0" in output
    assert "LED on 2" in output
    assert "LED off 2" in output
    assert "done" in output

    # Check GPIO traces - LED is on gpio_emul.00
    gpio_trace = parse_gpio_trace(circuitpython.trace_file, "gpio_emul.00")

    # Deduplicate by timestamp (keep last value at each timestamp)
    by_timestamp = {}
    for ts, val in gpio_trace:
        by_timestamp[ts] = val
    sorted_trace = sorted(by_timestamp.items())

    # Find transition points (where value changes), skipping initialization at ts=0
    transitions = []
    for i in range(1, len(sorted_trace)):
        prev_ts, prev_val = sorted_trace[i - 1]
        curr_ts, curr_val = sorted_trace[i]
        if prev_val != curr_val and curr_ts > 0:
            transitions.append((curr_ts, curr_val))

    # We expect at least 6 transitions (3 on + 3 off) from the blink loop
    assert len(transitions) >= 6, f"Expected at least 6 transitions, got {len(transitions)}"

    # Verify timing between consecutive transitions
    # Each sleep is 0.1s = 100ms = 100,000,000 ns
    expected_interval_ns = 100_000_000
    tolerance_ns = 20_000_000  # 20ms tolerance

    # Find a sequence of 6 consecutive transitions with ~100ms intervals (the blink loop)
    # This filters out initialization and cleanup noise
    blink_transitions = []
    for i in range(len(transitions) - 1):
        interval = transitions[i + 1][0] - transitions[i][0]
        if abs(interval - expected_interval_ns) < tolerance_ns:
            if not blink_transitions:
                blink_transitions.append(transitions[i])
            blink_transitions.append(transitions[i + 1])
        elif blink_transitions:
            # Found end of blink sequence
            break

    assert len(blink_transitions) >= 6, (
        f"Expected at least 6 blink transitions with ~100ms intervals, got {len(blink_transitions)}"
    )

    # Verify timing between blink transitions
    for i in range(1, min(6, len(blink_transitions))):
        prev_ts = blink_transitions[i - 1][0]
        curr_ts = blink_transitions[i][0]
        interval = curr_ts - prev_ts
        assert abs(interval - expected_interval_ns) < tolerance_ns, (
            f"Transition interval {interval / 1_000_000:.1f}ms deviates from "
            f"expected {expected_interval_ns / 1_000_000:.1f}ms by more than "
            f"{tolerance_ns / 1_000_000:.1f}ms tolerance"
        )


# --- PTY Input Tests ---


INPUT_CODE = """\
import sys

print("ready")
char = sys.stdin.read(1)
print(f"received: {repr(char)}")
print("done")
"""


@pytest.mark.circuitpy_drive({"code.py": INPUT_CODE})
def test_basic_serial_input(circuitpython):
    """Test reading single character from serial via PTY write."""
    circuitpython.serial.wait_for("ready")
    circuitpython.serial.write("A")
    circuitpython.wait_until_done()

    output = circuitpython.serial.all_output
    assert "ready" in output
    assert "received: 'A'" in output
    assert "done" in output


INPUT_FUNC_CODE = """\
print("ready")
name = input("Enter name: ")
print(f"hello {name}")
print("done")
"""


@pytest.mark.circuitpy_drive({"code.py": INPUT_FUNC_CODE})
def test_input_function(circuitpython):
    """Test the built-in input() function with PTY input."""
    circuitpython.serial.wait_for("Enter name:")
    circuitpython.serial.write("World\r")
    circuitpython.wait_until_done()

    output = circuitpython.serial.all_output
    assert "ready" in output
    assert "Enter name:" in output
    assert "hello World" in output
    assert "done" in output


INTERRUPT_CODE = """\
import time

print("starting")
for i in range(100):
    print(f"loop {i}")
    time.sleep(0.1)
print("completed")
"""


@pytest.mark.circuitpy_drive({"code.py": INTERRUPT_CODE})
def test_ctrl_c_interrupt(circuitpython):
    """Test sending Ctrl+C (0x03) to interrupt running code."""
    circuitpython.serial.wait_for("loop 5")
    circuitpython.serial.write("\x03")
    circuitpython.wait_until_done()

    output = circuitpython.serial.all_output
    assert "starting" in output
    assert "loop 5" in output
    assert "KeyboardInterrupt" in output
    assert "completed" not in output


RELOAD_CODE = """\
print("first run")
import time
time.sleep(1)
print("done")
"""


@pytest.mark.circuitpy_drive({"code.py": RELOAD_CODE})
@pytest.mark.code_py_runs(2)
def test_ctrl_d_soft_reload(circuitpython):
    """Test sending Ctrl+D (0x04) to trigger soft reload."""
    circuitpython.serial.wait_for("first run")
    circuitpython.serial.write("\x04")
    circuitpython.wait_until_done()

    # Should see "first run" appear multiple times due to reload
    # or see a soft reboot message
    output = circuitpython.serial.all_output
    assert "first run" in output
    # The soft reload should restart the code before "done" is printed
    assert "done" in output
    assert output.count("first run") > 1
