#!/bin/sh
cd $(dirname "$0")
find ./ -depth -print | cpio -ov > ../tree.cpio
