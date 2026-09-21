#include "alembic_exporter.h"

#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>

#include <stdexcept>
#include <utility>

using namespace Alembic::AbcGeom;

struct VisionAlembicExporter::Impl {
    std::unique_ptr<OArchive> archive;
    std::unique_ptr<OPoints> points;
    size_t frame_count = 0;
    bool closed = false;

    Impl(const std::string& output_path, double fps)
    {
        if (output_path.empty()) {
            throw std::invalid_argument("Alembic: chemin de sortie vide.");
        }

        if (!(fps > 0.0)) {
            throw std::invalid_argument("Alembic: cadence invalide.");
        }

        archive = std::make_unique<OArchive>(
            Alembic::AbcCoreOgawa::WriteArchive(),
            output_path
        );

        TimeSamplingPtr time_sampling(
            new TimeSampling(1.0 / fps, 0.0)
        );

        points = std::make_unique<OPoints>(
            archive->getTop(),
            "face_points",
            time_sampling
        );
    }
};

VisionAlembicExporter::VisionAlembicExporter(
    const std::string& output_path,
    double frames_per_second
)
    : impl_(std::make_unique<Impl>(output_path, frames_per_second))
{
}

VisionAlembicExporter::~VisionAlembicExporter()
{
    close();
}

void VisionAlembicExporter::write_frame(
    const std::vector<Landmark>& points,
    const std::vector<uint32_t>& point_ids
)
{
    if (!impl_ || impl_->closed) {
        throw std::runtime_error("Alembic: exporteur déjà fermé.");
    }

    if (points.empty()) {
        throw std::invalid_argument("Alembic: frame sans points.");
    }

    if (!point_ids.empty() && point_ids.size() != points.size()) {
        throw std::invalid_argument("Alembic: nombre d'identifiants incohérent.");
    }

    std::vector<V3f> positions;
    positions.reserve(points.size());

    std::vector<Alembic::Util::uint64_t> ids;
    ids.reserve(points.size());

    for (size_t i = 0; i < points.size(); ++i) {
        positions.emplace_back(
            points[i][0],
            points[i][1],
            points[i][2]
        );

        ids.push_back(
            point_ids.empty()
                ? static_cast<Alembic::Util::uint64_t>(i)
                : static_cast<Alembic::Util::uint64_t>(point_ids[i])
        );
    }

    OPointsSchema::Sample sample(
        P3fArraySample(positions),
        UInt64ArraySample(ids)
    );

    impl_->points->getSchema().set(sample);
    ++impl_->frame_count;
}

void VisionAlembicExporter::close()
{
    if (!impl_ || impl_->closed) return;

    impl_->points.reset();
    impl_->archive.reset();
    impl_->closed = true;
}

size_t VisionAlembicExporter::frames_written() const noexcept
{
    return impl_ ? impl_->frame_count : 0;
}
