FROM placeholder-will-be-replaced-with-CI-variable

ARG ASETUP_VERSION="placeholder-will-be-replaced-with-CI-variable"
ARG DEBIAN_FRONTEND=noninteractive

COPY . /tmp/source
COPY ./python /opt/fastframes/python
COPY ./python_wrapper /opt/fastframes/python_wrapper

RUN mkdir -p /opt/fastframes /tmp/build && \
    export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase && \
    source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh --quiet && \
    asetup $ASETUP_VERSION && \
    cmake -S /tmp/source -B /tmp/build -DCMAKE_INSTALL_PREFIX=/opt/fastframes && \
    cd /tmp/build && \
    make -j 4 && \
    make install && \
    cd && \
    rm -rf /tmp && \
    ldd /opt/fastframes/lib/libFastFrames.so

ENV CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:+$CMAKE_PREFIX_PATH:}/opt/fastframes"
ENV PATH="/opt/fastframes/bin:/opt/fastframes/python:${PATH}"
ENV LD_LIBRARY_PATH="/opt/fastframes/lib:${LD_LIBRARY_PATH}"
ENV ROOT_INCLUDE_PATH="${ROOT_INCLUDE_PATH:+$ROOT_INCLUDE_PATH:}/opt/fastframes/include"
