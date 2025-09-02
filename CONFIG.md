# ReactOS Build Configuration Guide

This guide explains how to use the `configure.sh` script to set up your ReactOS build environment.

## Prerequisites

- Ensure the environment variable `ROS_ARCH` is set to your target architecture (e.g., `x86`, `x86_64`).
- You need `cmake` and either `ninja` or `make` installed.
- This script is designed to work primarily with the MinGW build environment.

## Usage

Run the `configure.sh` script from the ReactOS source directory:

```sh
./configure.sh [options]