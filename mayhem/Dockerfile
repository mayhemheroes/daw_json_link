# syntax=docker/dockerfile:1.7
# Commit-image Dockerfile for daw_json_link (header-only C++17 JSON library).
FROM ghcr.io/mayhemheroes/base:20260621

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

# Pre-clone the header-only dependencies + test corpus at pinned SHAs so mayhem/build.sh
# is air-gapped (FetchContent is pointed at these trees, fully disconnected). Boost is
# optional for a few tests (guarded by Boost_FOUND upstream).
USER root
RUN [ -n "$SOURCE_DATE_EPOCH" ] && repro-sources-list.sh || true ; \
    apt-get update && apt-get install -yq --no-install-recommends \
      libboost-container-dev libboost-system-dev \
 && rm -rf /var/lib/apt/lists/*
RUN mkdir -p /opt/daw-deps && \
    git clone https://github.com/beached/header_libraries.git /opt/daw-deps/header_libraries && \
    git -C /opt/daw-deps/header_libraries checkout -q 1c164c3e1d814f181f05a51cdb1d4abea2b9bf72 && \
    git clone https://github.com/beached/utf_range.git /opt/daw-deps/utf_range && \
    git -C /opt/daw-deps/utf_range checkout -q b0b9cd05ea00f73d51cda5e3a2fd8fb004f5881d && \
    git clone https://github.com/beached/daw_json_link_test_data.git /opt/daw-deps/daw_json_link_test_data && \
    git -C /opt/daw-deps/daw_json_link_test_data checkout -q 66599b269b9c3301a6590e848ab6840ed3c9edd3 && \
    chown -R 2000:2000 /opt/daw-deps

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN bash mayhem/build.sh

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh — repo ships no functional tests"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
