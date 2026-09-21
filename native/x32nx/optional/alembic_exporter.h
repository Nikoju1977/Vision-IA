#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class VisionAlembicExporter {
public:
    using Landmark = std::array<float, 3>;

    explicit VisionAlembicExporter(
        const std::string& output_path,
        double frames_per_second = 30.0
    );

    ~VisionAlembicExporter();

    VisionAlembicExporter(const VisionAlembicExporter&) = delete;
    VisionAlembicExporter& operator=(const VisionAlembicExporter&) = delete;

    void write_frame(
        const std::vector<Landmark>& points,
        const std::vector<uint32_t>& point_ids = {}
    );

    void close();

    size_t frames_written() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
