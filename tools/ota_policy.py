"""Host-verifiable OTA safety policy shared by fault-injection tests."""

from dataclasses import dataclass


@dataclass(frozen=True)
class UpgradeContext:
    session_active: bool
    motor_running: bool
    storage_flushed: bool
    power_stable: bool
    signature_valid: bool
    target_matches: bool
    version_allowed: bool


def can_install(context: UpgradeContext) -> bool:
    return (
        not context.session_active
        and not context.motor_running
        and context.storage_flushed
        and context.power_stable
        and context.signature_valid
        and context.target_matches
        and context.version_allowed
    )

