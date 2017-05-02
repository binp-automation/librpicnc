#!/bin/sh

LD_LIBRARY_PATH=./pigpio RUST_BACKTRACE=1 ./target/debug/rpi-cnc
