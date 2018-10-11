#!/usr/bin/env python3

import sys
from shutil import copyfile

src = "../subprojects/tickle-pd-helpfile/tickle~-help.pd"
dst = "{}/tickle~-help.pd".format(sys.argv[1])
copyfile(src, dst)
