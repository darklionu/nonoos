FROM debian:trixie-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --no-install-recommends -y \
        g++ \
        libcurl4-openssl-dev \
        nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/nonoos

COPY rootfs/usr/src/ia/ ia/
COPY rootfs/usr/src/memory/ram_memory.h memory/ram_memory.h
COPY rootfs/usr/src/memory/ram_memory.cpp memory/ram_memory.cpp
COPY docker/smoke-test.sh /usr/local/bin/nonoos-smoke-test

RUN g++ -std=c++17 -Wall -Wextra -pedantic -Iia \
        ia/main.cpp ia/Chatbot.cpp ia/IntentDetector.cpp ia/TextProcessing.cpp \
        ia/WeatherService.CPP ia/UsbService.cpp \
        -o /usr/local/bin/chatia -lcurl \
    && g++ -std=c++17 -Wall -Wextra -pedantic -Imemory \
        memory/ram_memory.cpp -o /usr/local/bin/ram-memory \
    && chmod 0755 /usr/local/bin/nonoos-smoke-test

ENTRYPOINT ["/usr/local/bin/nonoos-smoke-test"]
