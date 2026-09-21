FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends     build-essential     cmake     libopencv-dev     libprotobuf-dev     protobuf-compiler     libssl-dev     libgtest-dev     ca-certificates     && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/vision_ia
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release     && cmake --build build --parallel     && ctest --test-dir build --output-on-failure

EXPOSE 8001/udp

CMD ["./build/vision_ia_node", "--port", "8001"]
