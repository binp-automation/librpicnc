#!/bin/sh

git submodule update --init --recursive

cd pigpio
make libs
cd ..

cargo build
