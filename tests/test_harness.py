#!/usr/bin/env python

__author__ = 'bhargava'

import os, sys, subprocess
import collections
import filecmp
import unittest

def execwrapper(args, errMsg):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True, executable="/bin/bash")
    out, err = p.communicate()
    return err

def diffExp(testname):
    filename = os.path.basename(testname)
    return (filecmp.cmp("nodes.csv", "expects/%s.nodes.exp" % filename) and
            filecmp.cmp("edges.csv", "expects/%s.edges.exp" % filename))

def invokecj(filename):
    args = "clang-joern -p . -ast-export %s" % filename
    return execwrapper(args, "Test on %s" % filename + " failed")

# Schema: filename, filename.nodes.exp, filename.edges.exp
def testinput(testname):
    if not invokecj(testname):
        ret = diffExp(testname)
    else:
        ret = False
    return ret

class SimpleTester(unittest.TestCase):
    def tearDown(self):
        tmpfiles = ['nodes.csv', 'edges.csv', '.nodeID']
        for f in tmpfiles:
            os.remove(f)

class TestBasic(SimpleTester):
    def test_basic(self):
        self.assertEqual(testinput('basic.c'), True,
                         'Test on basic.c failed')
    def test_arithmetic(self):
        self.assertEqual(testinput('arithmetic.c'), True,
                         'Test on arithmetic.c failed')

if __name__ == '__main__':
    unittest.main()