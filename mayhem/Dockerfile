# Build Stage
FROM --platform=linux/amd64 ubuntu:22.04 as builder

## Install build dependencies.
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y clang cmake git

## Add source code to the build stage.
ADD . /daw-json-link
WORKDIR /daw-json-link

## Build
RUN cmake . -DDAW_INSTRUMENT_MAYHEM=On && make -j$(nproc)

# Package Stage
FROM --platform=linux/amd64 ubuntu:22.04
COPY --from=builder /daw-json-link/mayhem/fuzz_daw /fuzz_serializer

## Set up fuzzing!
ENTRYPOINT []
CMD /fuzz_serializer
