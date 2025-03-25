# syntax=docker/dockerfile:1

FROM archlinux:latest

RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm \
    base-devel \
    gcc \
    gdb \
    valgrind \
    && pacman -Scc --noconfirm

WORKDIR /workspace

CMD ["/bin/bash"]
