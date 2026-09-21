#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "alembic_exporter.h"
#include "vision_engine.h"

namespace py = pybind11;

PYBIND11_MODULE(vision_ia, module)
{
    module.doc() =
        "Vision-IA VFX Core API: TFLite face landmarks + Alembic OPoints";

    py::class_<VisionAlembicExporter>(
        module,
        "AlembicExporter"
    )
        .def(
            py::init<const std::string&, double>(),
            py::arg("output_path"),
            py::arg("frames_per_second") = 30.0
        )
        .def(
            "write_frame",
            &VisionAlembicExporter::write_frame,
            py::arg("points"),
            py::arg("point_ids") = std::vector<uint32_t>{}
        )
        .def(
            "close",
            &VisionAlembicExporter::close
        )
        .def_property_readonly(
            "frames_written",
            &VisionAlembicExporter::frames_written
        );

    py::class_<VisionEngine>(
        module,
        "Engine"
    )
        .def(
            py::init<const std::string&>(),
            py::arg("model_path")
        )
        .def(
            "process_frame",
            &VisionEngine::process_frame
        )
        .def_property_readonly(
            "input_width",
            &VisionEngine::input_width
        )
        .def_property_readonly(
            "input_height",
            &VisionEngine::input_height
        )
        .def_property_readonly(
            "face_detector_ready",
            &VisionEngine::face_detector_ready
        );
}
