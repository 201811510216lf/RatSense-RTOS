from tools.ota_policy import UpgradeContext, can_install


def test_safe_window_accepts_valid_image():
    assert can_install(UpgradeContext(False, False, True, True, True, True, True))


def test_running_motor_rejects_upgrade():
    assert not can_install(UpgradeContext(False, True, True, True, True, True, True))


def test_bad_signature_rejects_upgrade():
    assert not can_install(UpgradeContext(False, False, True, True, False, True, True))

