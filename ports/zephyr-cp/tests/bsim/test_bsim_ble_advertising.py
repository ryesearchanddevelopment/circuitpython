# SPDX-FileCopyrightText: 2025 Scott Shawcroft for Adafruit Industries
# SPDX-License-Identifier: MIT

"""BLE advertising tests for nrf5340bsim."""

import logging
import time

import pytest

pytestmark = pytest.mark.circuitpython_board("native_nrf5340bsim")

logger = logging.getLogger(__name__)

BSIM_ADV_CODE = """\
import _bleio
import time

name = b"CPADV"
advertisement = bytes((2, 0x01, 0x06, len(name) + 1, 0x09)) + name

adapter = _bleio.adapter
print("adv start")
adapter.start_advertising(advertisement, connectable=False)
print("adv started")
time.sleep(4)
adapter.stop_advertising()
print("adv stop")
"""

BSIM_ADV_INTERRUPT_RELOAD_CODE = """\
import _bleio
import time

name = b"CPADV"
advertisement = bytes((2, 0x01, 0x06, len(name) + 1, 0x09)) + name

adapter = _bleio.adapter
print("adv run start")
adapter.start_advertising(advertisement, connectable=False)
print("adv running")
while True:
    time.sleep(0.2)
"""


@pytest.mark.zephyr_sample("bluetooth/observer")
@pytest.mark.circuitpy_drive({"code.py": BSIM_ADV_CODE})
def test_bsim_advertise_and_scan(bsim_phy, circuitpython, zephyr_sample):
    """Advertise from CircuitPython and verify Zephyr observer sees traffic."""
    observer = zephyr_sample

    start_time = time.time()
    while time.time() - start_time < 12.0:
        observer_output = observer.serial.all_output
        adv_ready = "adv started" in circuitpython.serial.all_output
        if (
            "Device found:" in observer_output
            and "AD data len 10" in observer_output
            and adv_ready
        ):
            break
        time.sleep(0.05)

    observer_output = observer.serial.all_output
    assert "adv start" in circuitpython.serial.all_output
    assert "adv started" in circuitpython.serial.all_output
    assert "Device found:" in observer_output
    assert "AD data len 10" in observer_output


@pytest.mark.zephyr_sample("bluetooth/observer", timeout=20.0)
@pytest.mark.circuitpy_drive({"code.py": BSIM_ADV_INTERRUPT_RELOAD_CODE})
def test_bsim_advertise_ctrl_c_reload(bsim_phy, circuitpython, zephyr_sample):
    """Ensure advertising resumes after Ctrl-C and a reload."""
    observer = zephyr_sample

    start_time = time.time()
    sent_ctrl_c = False
    sent_reload = False
    observer_count_before = 0

    while time.time() - start_time < 22.0:
        cp_output = circuitpython.serial.all_output
        observer_output = observer.serial.all_output
        device_found_count = observer_output.count("Device found:")

        if not sent_ctrl_c and "adv running" in cp_output and device_found_count > 0:
            circuitpython.serial.write("\x03")
            sent_ctrl_c = True
            observer_count_before = device_found_count

        if sent_ctrl_c and not sent_reload and "KeyboardInterrupt" in cp_output:
            circuitpython.serial.write("\x04")
            sent_reload = True

        if sent_reload and cp_output.count("adv running") >= 2:
            break

        time.sleep(0.05)

    cp_output = circuitpython.serial.all_output
    observer_output = observer.serial.all_output
    logger.info(observer_output)
    logger.info(cp_output)

    assert "adv run start" in cp_output
    assert "KeyboardInterrupt" in cp_output
    assert cp_output.count("adv running") >= 2
    assert observer_output.count("Device found:") >= observer_count_before
    assert "Already advertising" not in cp_output
