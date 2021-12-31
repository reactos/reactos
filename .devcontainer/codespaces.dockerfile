# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.195.0/containers/cpp/.devcontainer/base.Dockerfile
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-debian-11

USER vscode

WORKDIR /home/vscode/

RUN sudo apt-get -q update && \
    sudo apt-get -yq upgrade && \
    sudo apt-get -yq install qemu-system-x86 qemu-utils gdb-mingw-w64 && \
    sudo rm -rf /var/lib/apt/lists/*

RUN wget https://svn.reactos.org/amine/RosBEBinFull.tar.gz && \
    sudo tar -xzvf RosBEBinFull.tar.gz -C /usr/local --one-top-level=RosBE --strip-components 1 && \
    rm -f RosBEBinFull.tar.gz

RUN echo 'export PATH=/usr/local/RosBE/i386/bin:$PATH' >> /home/vscode/.profile

RUN qemu-img create -f qcow2 reactos_hdd.qcow 10G
