# SPDX-FileCopyrightText: 2025 Scott Shawcroft for Adafruit Industries
# SPDX-License-Identifier: MIT

"""Basic BabbleSim connectivity tests for nrf5340bsim."""

import pytest

pytestmark = pytest.mark.circuitpython_board("native_nrf5340bsim")

BSIM_CODE = """\
print("bsim ready")
"""


@pytest.mark.circuitpy_drive({"code.py": BSIM_CODE})
@pytest.mark.circuitpy_drive({"code.py": BSIM_CODE})
def test_bsim_dual_instance_connect(bsim_phy, circuitpython1, circuitpython2):
    """Run two bsim instances on the same sim id and verify UART output."""
    print("in the test")

    # Wait for both devices to produce their expected output before
    # tearing down the simulation.
    circuitpython1.serial.wait_for("bsim ready")
    circuitpython2.serial.wait_for("bsim ready")

    bsim_phy.finish_sim()

    output0 = circuitpython1.serial.all_output
    output1 = circuitpython2.serial.all_output

    assert "Board ID:native_nrf5340bsim" in output0
    assert "Board ID:native_nrf5340bsim" in output1
    assert "bsim ready" in output0
    assert "bsim ready" in output1
