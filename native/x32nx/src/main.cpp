#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "x32nx.pb.h"

namespace {

int parse_port(
    int argc,
    char** argv
)
{
    int port = 8001;

    for (int i = 1; i + 1 < argc; ++i) {
        if (
            std::string(argv[i]) ==
            "--port"
        ) {
            port = std::stoi(
                argv[i + 1]
            );
        }
    }

    if (
        port < 1 ||
        port > 65535
    ) {
        throw std::invalid_argument(
            "invalid UDP port"
        );
    }

    return port;
}

}

int main(
    int argc,
    char** argv
)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    try {
        const int port =
            parse_port(argc, argv);

        const int socket_fd =
            socket(
                AF_INET,
                SOCK_DGRAM,
                IPPROTO_UDP
            );

        if (socket_fd < 0) {
            throw std::runtime_error(
                std::string("socket(): ") +
                std::strerror(errno)
            );
        }

        sockaddr_in address{};

        address.sin_family =
            AF_INET;

        address.sin_addr.s_addr =
            htonl(INADDR_ANY);

        address.sin_port =
            htons(
                static_cast<uint16_t>(
                    port
                )
            );

        if (
            bind(
                socket_fd,
                reinterpret_cast<
                    const sockaddr*
                >(&address),
                sizeof(address)
            ) < 0
        ) {
            const std::string reason =
                std::strerror(errno);

            close(socket_fd);

            throw std::runtime_error(
                "bind(): " + reason
            );
        }

        std::cout
            << "[Vision-IA X32-NX] UDP/"
            << port
            << " ready"
            << std::endl;

        std::vector<uint8_t> buffer(
            65535
        );

        for (;;) {
            sockaddr_in peer{};

            socklen_t peer_length =
                sizeof(peer);

            const ssize_t bytes =
                recvfrom(
                    socket_fd,
                    buffer.data(),
                    buffer.size(),
                    0,
                    reinterpret_cast<
                        sockaddr*
                    >(&peer),
                    &peer_length
                );

            if (bytes < 0) {
                if (errno == EINTR) {
                    continue;
                }

                std::cerr
                    << "[UDP] "
                    << std::strerror(errno)
                    << std::endl;

                continue;
            }

            x32nx::StreamPacket packet;

            if (
                !packet.ParseFromArray(
                    buffer.data(),
                    static_cast<int>(bytes)
                )
            ) {
                std::cerr
                    << "[X32-NX] invalid protobuf packet: "
                    << bytes
                    << " bytes"
                    << std::endl;

                continue;
            }

            std::cout
                << "[X32-NX] seq="
                << packet.sequence_number()
                << " video="
                << packet.video_payload().size()
                << "B audio="
                << packet.audio_payload().size()
                << "B ratio="
                << packet.video_allocation_ratio()
                << "%"
                << std::endl;
        }
    }
    catch (
        const std::exception& error
    ) {
        std::cerr
            << "[Vision-IA X32-NX] "
            << error.what()
            << std::endl;

        google::protobuf::
            ShutdownProtobufLibrary();

        return 1;
    }
}
