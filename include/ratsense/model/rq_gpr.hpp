#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ratsense::model {

constexpr std::size_t kFeatureCount = 10U;

enum class ModelStatus { Valid, InvalidVersion, InvalidCrc, InvalidInput };

struct ModelParameters {
    std::uint32_t version{1};
    std::array<float, kFeatureCount> mean{};
    std::array<float, kFeatureCount> scale{};
    std::array<float, kFeatureCount> support{};
    std::array<float, kFeatureCount> weights{};
    float alpha{0.05F};
    float output_mean{36.5F};
    float output_scale{1.0F};
    std::uint32_t crc{0};
};

struct Prediction {
    float value{0.0F};
    float uncertainty{0.0F};
    ModelStatus status{ModelStatus::InvalidVersion};
};

class RqGprModel {
public:
    explicit RqGprModel(ModelParameters parameters = synthetic_parameters());
    static ModelParameters synthetic_parameters();
    bool validate() const;
    Prediction predict(const std::array<float, kFeatureCount>& features) const;
    const ModelParameters& parameters() const { return params_; }
    const std::string& feature_order() const { return feature_order_; }

private:
    ModelParameters params_;
    std::string feature_order_{"mean,max,min,std,center_mean,edge_mean,hot_fraction,ambient,humidity,quality"};
};

std::uint32_t model_crc(const ModelParameters& parameters);
const char* model_status_name(ModelStatus status);

}  // namespace ratsense::model

