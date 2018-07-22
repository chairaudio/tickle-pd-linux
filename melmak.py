#!/usr/bin/env python3

import os
import argparse
from subprocess import call, check_output

current_version = ""


def build():
    # configure?
    if not os.path.isdir("./build"):
        call(["env", "CXX=g++-7", "meson", "build"])

    # recreate version info
    # todo: check if gitversion differs from current_version
    gitversion_h = open("./src/gitversion.h", "w")
    gitversion_h.write("#define GITVERSION \"{}\"".format(current_version))
    gitversion_h.close()

    # build
    call(["ninja", "-C", "build", "install"])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Melmak is a metamaker.')
    parser.add_argument('--tag', nargs='?', default='*',
                        help='tag a release')
    args = parser.parse_args()

    # tag it?
    current_version = check_output(["git", "describe"]).decode().strip()
    new_version = None
    if args.tag != "*" and args.tag != None:
        # todo: provide shortcuts to autogenerate the version number
        if args.tag == "+":
            print("increase z")
        elif args.tag == "+y":
            print("increase y")
        else:
            print("tagging {}".format(args.tag))
            new_version = args.tag

    if new_version:
        call(["git", "tag", new_version, "-m", "release {}".format(new_version)])
        current_version = new_version
    build()
