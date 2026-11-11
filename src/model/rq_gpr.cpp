#include "ratsense/model/rq_gpr.hpp"

#include "ratsense/protocol/frame.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace ratsense::model {
namespace {

template <typename T>
void append_bytes(std::vector<std::uint8_t>& out, const T& value) {
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
    out.insert(out.end(), bytes, bytes + sizeof(T));
}

}  // namespace

ModelParameters RqGprModel::synthetic_parameters() {
    ModelParameters p{};
    p.version = 1;
    p.mean.fill(0.0F);
    p.scale.fill(1.0F);
    p.support.fill(0.0F);
    p.weights = {0.55F, 0.10F, -0.08F, 0.04F, 0.18F, -0.10F, 0.05F, 0.02F, 0.01F, 0.06F};
    p.alpha = 0.05F;
    p.output_mean = 36.5F;
    p.output_scale = 1.0F;
    p.crc = model_crc(p);
    return p;
}

RqGprModel::RqGprModel(ModelParameters parameters) : params_(parameters) {
    if (params_.crc == 0U) {
        params_.crc = model_crc(params_);
    }
}

std::uint32_t model_crc(const ModelParameters& parameters) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(4U + (kFeatureCount * 4U * 4U) + 12U);
    append_bytes(bytes, parameters.version);
    for (float value : parameters.mean) {
        append_bytes(bytes, value);
    }
    for (float value : parameters.scale) {
        append_bytes(bytes, value);
    }
    for (float value : parameters.support) {
        append_bytes(bytes, value);
    }
    for (float value : parameters.weights) {
        append_bytes(bytes, value);
    }
    append_bytes(bytes, parameters.alpha);
    append_bytes(bytes, parameters.output_mean);
    append_bytes(bytes, parameters.output_scale);
    return protocol::crc32_ieee(bytes);
}

bool RqGprModel::validate() const {
    if (params_.version == 0U || params_.crc != model_crc(params_)) {
        return false;
    }
    for (float scale : params_.scale) {
        if (!std::isfinite(scale) || scale == 0.0F) {
            return false;
        }
    }
    return std::isfinite(params_.alpha) && params_.alpha >= 0.0F && std::isfinite(params_.output_scale) &&
           params_.output_scale > 0.0F;
}

Prediction RqGprModel::predict(const std::array<float, kFeatureCount>& features) const {
    if (!validate()) {
        return Prediction{0.0F, 0.0F, params_.version == 0U ? ModelStatus::InvalidVersion : ModelStatus::InvalidCrc};
    }
    float result = 0.0F;
    float distance = 0.0F;
    for (std::size_t i = 0; i < kFeatureCount; ++i) {
        if (!std::isfinite(features[i])) {
            return Prediction{0.0F, 0.0F, ModelStatus::InvalidInput};
        }
        const float normalized = (features[i] - params_.mean[i]) / params_.scale[i];
        result += params_.weights[i] * normalized;
        distance += normalized * normalized;
    }
    const float kernel = std::exp(-0.5F * distance);
    const float value = params_.output_mean + params_.output_scale * (result * kernel);
    const float uncertainty = params_.alpha + (1.0F - kernel);
    return Prediction{value, uncertainty, ModelStatus::Valid};
}

const char* model_status_name(ModelStatus status) {
    switch (status) {
        case ModelStatus::Valid:
            return "valid";
        case ModelStatus::InvalidVersion:
            return "invalid_version";
        case ModelStatus::InvalidCrc:
            return "invalid_crc";
        case ModelStatus::InvalidInput:
            return "invalid_input";
    }
    return "unknown";
}

}  // namespace ratsense::model

