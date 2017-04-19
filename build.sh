#!/bin/sh

git submodule update --init --recursive

cd pigpio
make lib
cd ..

cargo build
