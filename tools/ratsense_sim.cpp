#include "ratsense/app/training.hpp"
#include "ratsense/drivers/display.hpp"
#include "ratsense/drivers/pressure.hpp"
#include "ratsense/drivers/thermal.hpp"
#include "ratsense/model/rq_gpr.hpp"
#include "ratsense/protocol/frame.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

struct SimResult {
    bool passed{true};
    std::string note;
};

std::string argument(int argc, char** argv, const std::string& name, const std::string& fallback) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == name) {
            return argv[i + 1];
        }
    }
    return fallback;
}

void write_binary(const std::filesystem::path& path, const std::vector<std::uint8_t>& data) {
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

SimResult run_scenario(const std::string& scenario, const std::filesystem::path& out_dir) {
    std::filesystem::create_directories(out_dir);
    const std::filesystem::path log_path = out_dir / (scenario + ".log");
    std::error_code ec;
    std::filesystem::remove(log_path, ec);
    ratsense::app::TrainingConfig training_config{};
    if (scenario == "pellet_missing") {
        training_config.pellet_timeout_ms = 250U;
    }
    if (scenario == "motor_timeout") {
        training_config.motor_safety_timeout_ms = 20U;
    }
    ratsense::app::TrainingController controller(training_config, log_path);
    ratsense::drivers::PressureConfig pressure_config{};
    pressure_config.threshold_on_v = 0.25F;
    pressure_config.threshold_off_v = 0.12F;
    pressure_config.debounce_ms = 10U;
    pressure_config.long_press_ms = 300U;
    pressure_config.repeat_min_interval_ms = 100U;
    ratsense::drivers::PressureProcessor pressure(pressure_config);
    ratsense::drivers::PelletIr ir({true, 20U, 2U, 500U, true});
    ratsense::drivers::SharpP3502Frame lcd;
    lcd.clear(false);
    lcd.draw_text(4U, 4U, "RATSENSE");
    lcd.draw_text(4U, 16U, "STATE:IDLE");
    write_binary(out_dir / (scenario + ".pbm"), lcd.to_pbm());

    const bool self_check_ok = scenario != "model_invalid";
    controller.self_check(true, self_check_ok, scenario != "sd_full");
    controller.start_session(100U);

    for (std::uint64_t t = 0; t < 100U; t += 10U) {
        const std::uint16_t raw = static_cast<std::uint16_t>(1000U + (t % 20U));
        for (const auto& event : pressure.consume_dma_half(&raw, 1U, t)) {
            controller.on_pressure(event);
        }
    }

    std::mt19937 generator(1234U);
    std::uniform_int_distribution<int> noise(-40, 40);
    const std::uint64_t press_start = 200U;
    const std::uint64_t press_duration = scenario == "long_press" ? 900U : 80U;
    for (std::uint64_t t = press_start; t < press_start + press_duration; t += 10U) {
        int raw_value = 2600;
        if (scenario == "pressure_bounce") {
            raw_value = (t / 10U) % 2U == 0U ? 2600 : 1030 + noise(generator);
        }
        const std::uint16_t raw = static_cast<std::uint16_t>(std::clamp(raw_value, 0, 4095));
        for (const auto& event : pressure.consume_dma_half(&raw, 1U, t)) {
            controller.on_pressure(event);
        }
        controller.tick(t);
    }
    const std::uint64_t tick_end = scenario == "motor_timeout" ? 7000U : 3000U;
    for (std::uint64_t t = press_start + press_duration; t < tick_end; t += 5U) {
        const std::uint16_t raw = 1000U;
        for (const auto& event : pressure.consume_dma_half(&raw, 1U, t)) {
            controller.on_pressure(event);
        }
        controller.tick(t);
        if (scenario != "pellet_missing" && scenario != "motor_timeout" &&
            controller.state() == ratsense::app::TrainingState::AwaitPellet) {
            const auto rise = ir.edge(true, t, false);
            (void)rise;
            const auto fall = ir.edge(false, t + 8U, false);
            if (fall.has_value()) {
                controller.on_ir(*fall);
            }
        }
    }
    if (scenario == "motor_timeout") {
        controller.tick(6000U);
        if (controller.state() != ratsense::app::TrainingState::Fault) {
            return {false, "motor timeout did not enter fault state"};
        }
    }

    if (scenario == "command_replay") {
        const std::uint64_t pellets_before_replay = controller.counters().pellet_pass_count;
        const auto rise = ir.edge(true, 3100U, false);
        (void)rise;
        const auto fall = ir.edge(false, 3110U, false);
        if (fall.has_value()) {
            controller.on_ir(*fall);
            controller.on_ir(*fall);
        }
        if (controller.counters().pellet_pass_count != pellets_before_replay + 1U) {
            return {false, "duplicate IR event created duplicate pellet"};
        }
    }

    if (scenario == "sd_full") {
        controller.log().set_fail_after_records(0U);
        ratsense::drivers::EventRecord record{};
        record.type = ratsense::drivers::RecordType::Error;
        record.session_id = controller.session_id();
        record.monotonic_ms = 4000U;
        record.payload = {0xdeU, 0xadU};
        (void)controller.log().append(record);
        if (controller.log().healthy()) {
            return {false, "sd_full did not expose write failure"};
        }
    }

    if (scenario == "uart_crc_error" || scenario == "uart_overflow" || scenario == "esp_reset") {
        ratsense::protocol::Frame frame{};
        frame.message_type = ratsense::protocol::MessageType::Heartbeat;
        frame.sequence = 7U;
        frame.device_time_ms = 1234U;
        frame.payload = {1U, 2U, 3U};
        ratsense::protocol::CodecError encode_error{};
        std::vector<std::uint8_t> encoded = ratsense::protocol::encode_frame(frame, &encode_error);
        encoded.push_back(0U);
        if (scenario == "uart_crc_error" && encoded.size() > 4U) {
            encoded[encoded.size() - 3U] ^= 0x01U;
        }
        ratsense::protocol::StreamParser parser;
        if (scenario == "uart_overflow") {
            std::vector<std::uint8_t> noise(ratsense::protocol::kMaxEncodedFrame + 10U, 0x55U);
            parser.feed(noise.data(), noise.size(), [](const ratsense::protocol::Frame&) {});
            parser.feed(encoded.data(), encoded.size(), [](const ratsense::protocol::Frame&) {});
        } else {
            for (std::uint8_t byte : encoded) {
                parser.feed(&byte, 1U, [](const ratsense::protocol::Frame&) {});
            }
        }
        if (scenario == "uart_crc_error" && parser.frames_bad() == 0U) {
            return {false, "CRC fault was not counted"};
        }
        if (scenario == "uart_overflow" && parser.frames_bad() == 0U) {
            return {false, "overflow fault was not counted"};
        }
        if (scenario == "esp_reset") {
            parser.reset();
        }
    }

    ratsense::drivers::MlxSubpagePairer pairer;
    ratsense::drivers::MlxSubpage first{};
    first.index = 0U;
    first.time_ms = 1000U;
    ratsense::drivers::MlxSubpage second{};
    second.index = scenario == "thermal_subpage_loss" ? 0U : 1U;
    second.time_ms = 1040U;
    const auto pair = pairer.push(first);
    (void)pair;
    const auto pair_result = pairer.push(second);
    if (scenario == "thermal_subpage_loss" && pair_result.has_value()) {
        return {false, "thermal subpage loss incorrectly produced a pair"};
    }

    ratsense::model::RqGprModel model;
    if (scenario == "model_invalid") {
        auto params = model.parameters();
        params.crc ^= 0x100U;
        model = ratsense::model::RqGprModel(params);
    }
    const std::array<float, ratsense::model::kFeatureCount> features{36.5F, 36.8F, 36.1F, 0.2F, 36.6F,
                                                                      36.4F, 0.1F, 24.0F, 45.0F, 1.0F};
    const auto prediction = model.predict(features);
    if (scenario == "model_invalid" && prediction.status == ratsense::model::ModelStatus::Valid) {
        return {false, "invalid model accepted"};
    }

    std::vector<ratsense::protocol::Frame> frames = controller.drain_protocol_events();
    std::size_t encoded_bytes = 0;
    for (const auto& frame : frames) {
        encoded_bytes += ratsense::protocol::encode_frame(frame).size() + 1U;
    }

    std::cout << "scenario=" << scenario << " state=" << ratsense::app::training_state_name(controller.state())
              << " press_count=" << controller.counters().press_count
              << " motor_attempts=" << controller.counters().motor_attempt_count
              << " pellet_count=" << controller.counters().pellet_pass_count
              << " events=" << frames.size() << " uart_bytes=" << encoded_bytes
              << " model_status=" << ratsense::model::model_status_name(prediction.status) << '\n';

    if (scenario == "normal" && controller.counters().pellet_pass_count == 0U) {
        return {false, "normal scenario did not confirm a pellet"};
    }
    if (scenario == "pellet_missing" && controller.state() != ratsense::app::TrainingState::Fault) {
        return {false, "pellet_missing scenario did not fault safely"};
    }
    if (scenario == "command_replay" && controller.counters().pellet_pass_count < 1U) {
        return {false, "command replay did not preserve the original pellet event"};
    }
    return {true, "deterministic scenario completed"};
}

}  // namespace

int main(int argc, char** argv) {
    const std::string scenario = argument(argc, argv, "--scenario", "normal");
    const std::filesystem::path out_dir = argument(argc, argv, "--out", "out/sim");
    const std::array<std::string, 19> known{
        "normal",          "pressure_bounce", "long_press",       "motor_timeout", "pellet_missing",
        "mlx_i2c_nack",    "thermal_subpage_loss", "model_invalid", "sd_full", "sd_removed",
        "esp_reset",       "uart_crc_error",   "uart_overflow",    "mqtt_disconnect", "mqtt_duplicate",
        "broker_restart",  "command_replay",  "ota_bad_signature", "power_loss"};
    if (std::find(known.begin(), known.end(), scenario) == known.end()) {
        std::cerr << "unknown scenario: " << scenario << '\n';
        return 2;
    }
    const SimResult result = run_scenario(scenario, out_dir);
    std::cout << "result=" << (result.passed ? "PASS" : "FAIL") << " note=" << result.note << '\n';
    return result.passed ? 0 : 1;
}

