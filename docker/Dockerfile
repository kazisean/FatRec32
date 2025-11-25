FROM gcc:latest

WORKDIR /app

# update
RUN apt-get update && apt-get install -y \
    make \
    && rm -rf /var/lib/apt/lists/*

# keep the container running
CMD ["/bin/bash"]