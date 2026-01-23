# SPDX-FileCopyrightText: 2025 Scott Shawcroft for Adafruit Industries
# SPDX-License-Identifier: MIT

"""BLE scanning tests for nrf5340bsim."""

import time

import pytest

pytestmark = pytest.mark.circuitpython_board("native_nrf5340bsim")

BSIM_SCAN_CODE = """\
import _bleio

adapter = _bleio.adapter
print("scan start")
scan = adapter.start_scan(timeout=4.0, active=True)
found = False
for entry in scan:
    if b"zephyrproject" in entry.advertisement_bytes:
        print("found beacon")
        found = True
        break
adapter.stop_scan()
print("scan done", found)
"""

BSIM_SCAN_RELOAD_CODE = """\
import _bleio
import time

adapter = _bleio.adapter

print("scan run start")
for _ in range(10):
    try:
        scan = adapter.start_scan(timeout=4.0, active=True)
        break
    except OSError:
        time.sleep(0.1)
else:
    raise RuntimeError("scan start failed")
found = False
for entry in scan:
    if b"zephyrproject" in entry.advertisement_bytes:
        print("found beacon run")
        found = True
        break
adapter.stop_scan()
print("scan run done", found)
"""

BSIM_SCAN_RELOAD_NO_STOP_CODE = """\
import _bleio
import time

adapter = _bleio.adapter

print("scan run start")
for _ in range(10):
    try:
        scan = adapter.start_scan(timeout=4.0, active=True)
        break
    except OSError:
        time.sleep(0.1)
else:
    raise RuntimeError("scan start failed")
found = False
for entry in scan:
    if b"zephyrproject" in entry.advertisement_bytes:
        print("found beacon run")
        found = True
        break
print("scan run done", found)
"""


@pytest.mark.zephyr_sample("bluetooth/beacon")
@pytest.mark.circuitpy_drive({"code.py": BSIM_SCAN_CODE})
def test_bsim_scan_zephyr_beacon(bsim_phy, circuitpython, zephyr_sample):
    """Scan for Zephyr beacon sample advertisement using bsim."""
    _ = zephyr_sample

    start_time = time.time()
    while time.time() - start_time < 6.0:
        if "found beacon" in circuitpython.serial.all_output:
            break
        time.sleep(0.05)

    assert "scan start" in circuitpython.serial.all_output
    assert "found beacon" in circuitpython.serial.all_output


@pytest.mark.zephyr_sample("bluetooth/beacon", timeout=12.0)
@pytest.mark.circuitpy_drive({"code.py": BSIM_SCAN_RELOAD_CODE})
def test_bsim_scan_zephyr_beacon_reload(bsim_phy, circuitpython, zephyr_sample):
    """Scan for Zephyr beacon, soft reload, and scan again."""
    _ = zephyr_sample

    start_time = time.time()
    sent_reload = False
    while time.time() - start_time < 12.0:
        output = circuitpython.serial.all_output

        if (
            not sent_reload
            and "scan run done" in output
            and "Press any key to enter the REPL" in output
        ):
            time.sleep(0.2)
            circuitpython.serial.write("\x04")
            sent_reload = True

        if sent_reload and output.count("scan run done") >= 2:
            break
        time.sleep(0.05)

    output = circuitpython.serial.all_output
    assert "scan run start" in output
    assert output.count("found beacon run") >= 2
    assert output.count("scan run done") >= 2


@pytest.mark.zephyr_sample("bluetooth/beacon", timeout=12.0)
@pytest.mark.circuitpy_drive({"code.py": BSIM_SCAN_RELOAD_NO_STOP_CODE})
def test_bsim_scan_zephyr_beacon_reload_no_stop(bsim_phy, circuitpython, zephyr_sample):
    """Scan for Zephyr beacon without explicit stop, soft reload, and scan again."""
    _ = zephyr_sample

    start_time = time.time()
    sent_reload = False
    while time.time() - start_time < 12.0:
        output = circuitpython.serial.all_output

        if (
            not sent_reload
            and "scan run done" in output
            and "Press any key to enter the REPL" in output
        ):
            time.sleep(0.2)
            circuitpython.serial.write("\x04")
            sent_reload = True

        if sent_reload and output.count("scan run done") >= 2:
            break
        time.sleep(0.05)

    output = circuitpython.serial.all_output
    assert "scan run start" in output
    assert output.count("found beacon run") >= 2
    assert output.count("scan run done") >= 2
