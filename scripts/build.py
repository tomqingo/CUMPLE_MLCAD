#!/usr/bin/env python

import argparse
import os
import subprocess
from packaging import version

run_files = 'scripts/*'

def get_cmake_version():
    output = subprocess.check_output(['cmake', '--version']).decode('utf-8')
    line = output.splitlines()[0]
    version = line.split()[2]
    return(version)

if version.parse(get_cmake_version()) < version.parse("3.10.0"):
    exit(1)

def run(command):
    if args.print_commands:
        print(command)
    if os.system(command) is not 0:
        if not args.print_commands:
            print(command)
        quit()


# cmake & copy run files
# for sanitize, need to remove -static from src/CMakeLists.txt
mode_cmake_options = {
    None: '',
    'debug': '-DCMAKE_BUILD_TYPE=Debug',
    'release': '-DCMAKE_BUILD_TYPE=Release',
    'profile': '-DCMAKE_CXX_FLAGS=-pg',
    'release_profile': '-DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-pg',
    'asan': '-DCMAKE_CXX_FLAGS=-fsanitize=address',
    'debug_asan': '-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS=-fsanitize=address',
    'tsan': '-DCMAKE_CXX_FLAGS=-fsanitize=thread',
    'debug_tsan': '-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS=-fsanitize=thread'
}

# argparse
parser = argparse.ArgumentParser(description='Build Cumple in Linux')
parser.add_argument('-o', '--mode', choices=mode_cmake_options.keys(), default='release')
parser.add_argument('-c', '--cmake_options', default='')
parser.add_argument('-j', '--jobs', default='8')
parser.add_argument('-p', '--print_commands', action='store_true')
parser.add_argument('-b', '--build_dir', default='build')
parser.add_argument('-r', '--run_dir', default='run')
parser.add_argument('-s', '--static_compilation', action='store_true', default=False)
args = parser.parse_args()

# targets
build_targets = ['Cumple', 'partitionHyperGraph']

# static_compilation
if args.static_compilation:
    static_flag = "-DENABLE_STATIC_COMPILE=ON"
else:
    static_flag = "-DENABLE_STATIC_COMPILE=OFF"

run('cmake src -B{} {} {} {}'.format(args.build_dir,
                                  mode_cmake_options[args.mode], args.cmake_options, static_flag))
run('mkdir -p {}'.format(args.run_dir))
run('cp -u -R {} {}'.format(run_files, args.run_dir))

# make
for target in build_targets:
    run('cmake --build {} --target {} -j {}'.format(args.build_dir,
                                                   target, args.jobs))

# build io_map
run('g++ scripts/io_map.cxx -o scripts/io_map')
cp_targets = build_targets
for target in cp_targets:
    run('cp -f -u {}/{} {}'.format(args.build_dir, target, args.run_dir))
