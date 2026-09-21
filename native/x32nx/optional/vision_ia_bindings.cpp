#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "alembic_exporter.h"
#include "vision_engine.h"

namespace py = pybind11;

PYBIND11_MODULE(vision_ia, module)
{
    module.doc() =
        "Vision-IA optional VFX Core API";

    py::class_<VisionAlembicExporter>(
        module,
        "AlembicExporter"
    )
        .def(
            py::init<const std::string&>()
        )
        .def(
            "write_frame",
            &VisionAlembicExporter::write_frame
        );

    py::class_<VisionEngine>(
        module,
        "Engine"
    )
        .def(
            py::init<const std::string&>()
        )
        .def(
            "process_frame",
            &VisionEngine::process_frame
        );
}
