# ----------------------------------------------------
# Stage 1: Builder (Compiles the C++ code)
# ----------------------------------------------------
FROM ubuntu:22.04 AS builder

# Avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# 1. Install Build Tools & Libraries
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libpcap-dev \
    libhiredis-dev \
    nlohmann-json3-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# 2. Set working directory
WORKDIR /app

# 3. Copy source code
COPY CMakeLists.txt .
COPY include/ ./include/
COPY src/ ./src/

# 4. Compile
RUN mkdir build && cd build \
    && cmake .. \
    && make

# ----------------------------------------------------
# Stage 2: Runner (Runs the executable)
# ----------------------------------------------------
FROM ubuntu:22.04

WORKDIR /app

# Install runtime libraries (lighter than dev libraries)
RUN apt-get update && apt-get install -y \
    libpcap0.8 \
    libhiredis0.14 \
    && rm -rf /var/lib/apt/lists/*

# Copy the executable from the builder stage
COPY --from=builder /app/build/deeptrace_sniffer .

# Default Command
CMD ["./deeptrace_sniffer"]