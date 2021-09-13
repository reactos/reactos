FROM gitpod/workspace-full-vnc

USER gitpod

# Install custom tools, runtime, etc. using apt-get
# For example, the command below would install "bastet" - a command line tetris clone:
#
# RUN sudo apt-get -q update && #     sudo apt-get install -yq bastet && #     sudo rm -rf /var/lib/apt/lists/*
#
# More information: https://www.gitpod.io/docs/config-docker/
RUN sudo apt-get -q update && \
    sudo apt-get -yq upgrade && \
    sudo apt-get -yq install qemu-system-x86 qemu-utils gdb-mingw-w64 && \
    sudo rm -rf /var/lib/apt/lists/*

RUN wget https://svn.reactos.org/amine/RosBEBinFull.tar.gz && \
    sudo tar -xzf RosBEBinFull.tar.gz -C /usr/local && \
    sudo mv /usr/local/RosBEBinFull /usr/local/RosBE && \
    rm -f RosBEBinFull.tar.gz

RUN echo 'export PATH=/usr/local/RosBE/i386/bin:$PATH' >> /home/gitpod/.profile

RUN qemu-img create -f qcow2 reactos_hdd.qcow 10G
