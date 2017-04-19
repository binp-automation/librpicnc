#!/bin/sh

git submodule update --init --recursive

cd pigpio
make lib
cd ..

RUST_BACKTRACE=1 cargo build -vv
