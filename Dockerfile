# Multistage docker build, requires docker 17.05

# builder stage
FROM ubuntu:26.04 AS builder

RUN set -ex && \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get --no-install-recommends --yes install \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        git \
        pkg-config

WORKDIR /src
COPY . .

ARG NPROC
RUN set -ex && \
    git submodule init && git submodule update && \
    rm -rf build && \
    if [ -z "$NPROC" ] ; \
    then make -j$(nproc) depends target=x86_64-linux-gnu ; \
    else make -j$NPROC depends target=x86_64-linux-gnu ; \
    fi

# runtime stage
FROM ubuntu:26.04

ENV UID=999

RUN set -ex && \
    apt-get update && \
    apt-get --no-install-recommends --yes install ca-certificates && \
    apt-get clean && \
    rm -rf /var/lib/apt
COPY --from=builder /src/build/x86_64-linux-gnu/release/bin /usr/local/bin/

# Create incognito user
RUN useradd --system --user-group --uid $UID incognito && \
        mkdir -p /wallet /home/incognito/.incognito && \
        chown -R incognito:incognito /home/incognito/.incognito && \
        chown -R incognito:incognito /wallet

# Contains the blockchain
VOLUME /home/incognito/.incognito

# Generate your wallet via accessing the container and run:
# cd /wallet
# incognito-wallet-cli
VOLUME /wallet

EXPOSE 39001
EXPOSE 39002

# switch to user incognito
USER incognito

ENTRYPOINT ["incognitod"]
CMD ["--p2p-bind-ip=0.0.0.0", "--p2p-bind-port=39001", "--rpc-bind-ip=0.0.0.0", "--rpc-bind-port=39002", "--non-interactive", "--confirm-external-bind"]
