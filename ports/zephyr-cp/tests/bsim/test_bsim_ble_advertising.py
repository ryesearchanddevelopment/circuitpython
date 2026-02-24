# SPDX-FileCopyrightText: 2025 Scott Shawcroft for Adafruit Industries
# SPDX-License-Identifier: MIT

"""BLE advertising tests for nrf5340bsim."""

import logging

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
time.sleep(10)
adapter.stop_advertising()
print("adv run done")
"""


@pytest.mark.zephyr_sample("bluetooth/observer")
@pytest.mark.circuitpy_drive({"code.py": BSIM_ADV_CODE})
def test_bsim_advertise_and_scan(bsim_phy, circuitpython, zephyr_sample):
    """Advertise from CircuitPython and verify Zephyr observer sees traffic."""
    observer = zephyr_sample

    circuitpython.wait_until_done()

    cp_output = circuitpython.serial.all_output
    observer_output = observer.serial.all_output
    assert "adv start" in cp_output
    assert "adv started" in cp_output
    assert "adv stop" in cp_output
    assert "Device found:" in observer_output
    assert "AD data len 10" in observer_output


@pytest.mark.zephyr_sample("bluetooth/observer")
@pytest.mark.code_py_runs(2)
@pytest.mark.duration(25)
@pytest.mark.circuitpy_drive({"code.py": BSIM_ADV_INTERRUPT_RELOAD_CODE})
def test_bsim_advertise_ctrl_c_reload(bsim_phy, circuitpython, zephyr_sample):
    """Ensure advertising resumes after Ctrl-C and a reload."""
    observer = zephyr_sample

    circuitpython.serial.wait_for("adv running")
    observer.serial.wait_for("Device found:")
    observer_count_before = observer.serial.all_output.count("Device found:")

    circuitpython.serial.write("\x03")
    circuitpython.serial.wait_for("KeyboardInterrupt")

    circuitpython.serial.write("\x04")
    circuitpython.wait_until_done()

    cp_output = circuitpython.serial.all_output
    observer_output = observer.serial.all_output
    logger.info(observer_output)
    logger.info(cp_output)

    assert "adv run start" in cp_output
    assert "KeyboardInterrupt" in cp_output
    assert cp_output.count("adv running") >= 2
    assert cp_output.count("adv run done") >= 1
    assert observer_output.count("Device found:") >= observer_count_before + 1
    assert "Already advertising" not in cp_output
