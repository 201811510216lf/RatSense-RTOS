#include "ratsense/app/training.hpp"
#include "ratsense/drivers/display.hpp"
#include "ratsense/drivers/ir.hpp"
#include "ratsense/drivers/motor.hpp"
#include "ratsense/drivers/pressure.hpp"
#include "ratsense/drivers/storage.hpp"
#include "ratsense/drivers/thermal.hpp"
#include "ratsense/model/rq_gpr.hpp"
#include "ratsense/protocol/frame.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {
int failures = 0;

#define CHECK(condition)                                                                                           \
    do {                                                                                                           \
        if (!(condition)) {                                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #condition << '\n';                  \
            ++failures;                                                                                             \
        }                                                                                                          \
    } while (false)

void test_protocol() {
    ratsense::protocol::Frame frame{};
    frame.message_type = ratsense::protocol::MessageType::ThermalSample;
    frame.flags = ratsense::protocol::AckRequested;
    frame.sequence = 0x12345678U;
    frame.ack_sequence = 9U;
    frame.device_time_ms = 123456789U;
    frame.payload = {0U, 1U, 2U, 0xffU, 3U};
    const auto encoded = ratsense::protocol::encode_frame(frame);
    CHECK(!encoded.empty());
    for (const auto byte : encoded) {
        CHECK(byte != 0U);
    }
    const auto decoded = ratsense::protocol::decode_frame(encoded);
    CHECK(decoded);
    CHECK(decoded.frame.sequence == frame.sequence);
    CHECK(decoded.frame.payload == frame.payload);

    std::vector<std::uint8_t> stream;
    stream.push_back(0U);
    stream.insert(stream.end(), encoded.begin(), encoded.end());
    stream.push_back(0U);
    ratsense::protocol::StreamParser parser;
    std::size_t count = 0;
    for (std::size_t split = 0; split < stream.size();) {
        const std::size_t chunk = std::min<std::size_t>(3U, stream.size() - split);
        parser.feed(stream.data() + split, chunk, [&](const ratsense::protocol::Frame&) { ++count; });
        split += chunk;
    }
    CHECK(count == 1U);
    CHECK(parser.frames_bad() == 0U);
    auto corrupt = encoded;
    corrupt[corrupt.size() / 2U] ^= 0x80U;
    CHECK(!ratsense::protocol::decode_frame(corrupt));
}

void test_pressure_motor_ir() {
    ratsense::drivers::PressureConfig config{};
    config.threshold_on_v = 0.2F;
    config.threshold_off_v = 0.1F;
    config.debounce_ms = 5U;
    config.long_press_ms = 50U;
    config.repeat_min_interval_ms = 20U;
    ratsense::drivers::PressureProcessor pressure(config);
    for (std::uint64_t t = 0; t < 100U; t += 10U) {
        (void)pressure.process_sample(1000U, t);
    }
    const auto down = pressure.process_sample(2600U, 100U);
    CHECK(down.has_value());
    CHECK(down->type == ratsense::drivers::PressureEventType::PressDown);
    const auto long_press = pressure.process_sample(2600U, 200U);
    CHECK(long_press.has_value());
    CHECK(long_press->type == ratsense::drivers::PressureEventType::LongPress);
    std::optional<ratsense::drivers::PressureEvent> up;
    for (std::uint64_t now = 300U; now < 500U && !up.has_value(); now += 10U) {
        up = pressure.process_sample(1000U, now);
    }
    CHECK(up.has_value());
    CHECK(up->type == ratsense::drivers::PressureEventType::PressUp);
    CHECK(pressure.worst_case_dma_latency_ms() > 0U);

    ratsense::drivers::Motor28BYJ48 motor;
    CHECK(!motor.start(2U, true, 0U));
    CHECK(motor.enable());
    CHECK(motor.start(8U, true, 0U));
    std::size_t phase_count = 0;
    for (std::uint64_t now = 0; now < 100U && motor.state() == ratsense::drivers::MotorState::Running; now += 3U) {
        const auto event = motor.tick(now);
        if (event.type == ratsense::drivers::MotorEvent::Type::PhaseChanged) {
            ++phase_count;
        }
    }
    CHECK(motor.state() == ratsense::drivers::MotorState::Complete);
    const std::array<bool, 4> released{false, false, false, false};
    CHECK(motor.coil_outputs() == released);
    CHECK(phase_count >= 7U);

    ratsense::drivers::PelletIr ir({true, 10U, 2U, 100U, true});
    CHECK(!ir.edge(true, 0U));
    const auto pellet = ir.edge(false, 10U);
    CHECK(pellet.has_value());
    CHECK(ir.pellet_passed() == 1U);
    const auto ignored = ir.edge(true, 11U, true);
    (void)ignored;
    const auto removed = ir.edge(false, 20U, true);
    CHECK(removed.has_value());
    CHECK(ir.pellet_removed() == 1U);
}

void test_thermal_model_display_storage() {
    ratsense::drivers::MlxSubpagePairer pairer;
    ratsense::drivers::MlxSubpage a{};
    a.index = 0U;
    ratsense::drivers::MlxSubpage b{};
    b.index = 1U;
    CHECK(!pairer.push(a).has_value());
    CHECK(pairer.push(b).has_value());
    b.index = 2U;
    CHECK(!pairer.push(b).has_value());
    CHECK(pairer.dropped_subpages() == 1U);

    ratsense::drivers::ThermalFramePool pool;
    auto* fill = pool.acquire_fill();
    CHECK(fill != nullptr);
    CHECK(pool.publish_ready(fill));
    auto* ready = pool.acquire_ready();
    CHECK(ready == fill);
    CHECK(pool.release(ready));
    CHECK(pool.ready_count() == 0U);

    const std::uint8_t bytes[2] = {0xbeU, 0xefU};
    CHECK(ratsense::drivers::htu21d_crc_valid(0xbeefU, ratsense::drivers::htu21d_crc8(bytes, 2U)));
    ratsense::model::RqGprModel model;
    const std::array<float, ratsense::model::kFeatureCount> features{};
    CHECK(model.predict(features).status == ratsense::model::ModelStatus::Valid);
    auto parameters = model.parameters();
    parameters.crc ^= 0x1U;
    ratsense::model::RqGprModel invalid(parameters);
    CHECK(invalid.predict(features).status == ratsense::model::ModelStatus::InvalidCrc);

    ratsense::drivers::SharpP3502Frame lcd;
    lcd.clear(false);
    lcd.set_pixel(0U, 0U, true);
    CHECK(lcd.pixels()[0] == 0x80U);
    CHECK(lcd.make_row_command(1U, true).size() == ratsense::drivers::kLcdRowBytes + 3U);
    CHECK(lcd.to_pbm().size() > ratsense::drivers::kLcdHeight * ratsense::drivers::kLcdRowBytes);

    const auto path = std::filesystem::path("out") / "ratsense_unit.log";
    std::filesystem::create_directories(path.parent_path());
    std::error_code error;
    std::filesystem::remove(path, error);
    ratsense::drivers::LogStore store(path);
    ratsense::drivers::EventRecord record{};
    record.type = ratsense::drivers::RecordType::Press;
    record.session_id = 42U;
    record.monotonic_ms = 100U;
    record.payload = {1U, 2U, 3U};
    CHECK(store.append(record));
    CHECK(store.recover().size() == 1U);
    std::vector<ratsense::drivers::EventRecord> replay;
    CHECK(store.replay_from(1U, replay));
    CHECK(replay.size() == 1U);
    store.set_fail_after_records(1U);
    CHECK(!store.append(record));
}

void test_training_idempotency() {
    const auto path = std::filesystem::path("out") / "ratsense_training_unit.log";
    std::filesystem::create_directories(path.parent_path());
    std::error_code error;
    std::filesystem::remove(path, error);
    ratsense::app::TrainingController controller({}, path);
    CHECK(controller.self_check(true, true, true));
    CHECK(controller.start_session(100U));
    ratsense::drivers::PressureSample sample{};
    sample.time_ms = 200U;
    ratsense::drivers::PressureEvent press{ratsense::drivers::PressureEventType::PressDown, sample};
    controller.on_pressure(press);
    for (std::uint64_t now = 200U; now < 1000U; now += 3U) {
        controller.tick(now);
    }
    CHECK(controller.state() == ratsense::app::TrainingState::AwaitPellet);
    ratsense::drivers::IrEvent pellet{ratsense::drivers::IrEventType::PelletPassed, 1000U, 10U, 1U};
    controller.on_ir(pellet);
    controller.on_ir(pellet);
    CHECK(controller.counters().pellet_pass_count == 1U);
}

}  // namespace

int main() {
    test_protocol();
    test_pressure_motor_ir();
    test_thermal_model_display_storage();
    test_training_idempotency();
    if (failures != 0) {
        std::cerr << failures << " test checks failed\n";
        return 1;
    }
    std::cout << "all Host unit checks passed\n";
    return 0;
}

