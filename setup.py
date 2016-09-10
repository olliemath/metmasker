#!/usr/bin/env python2

from setuptools import setup, Extension
import numpy

setup(name="masker", version="1.0", ext_modules=[Extension(
    name="masker",
    sources=["masker.c", "loader.c", "algorithms.c"],
    include_dirs=[numpy.get_include()],
    libraries=["png"],
    extra_compile_args=['-Ofast', '-std=c99']
)])
