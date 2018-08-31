#!/usr/bin/env python
from distutils.core import setup, Extension
import subprocess

def pkgconfig(*packages, **kw):
    flag_map = {'-I': 'include_dirs', '-L': 'library_dirs', '-l': 'libraries'}
    for token in commands.getoutput("pkg-config --libs --cflags %s" % ' '.join(packages)).split():
        if token[:2] in flag_map:
            kw.setdefault(flag_map.get(token[:2]), []).append(token[2:])
        else:
            kw.setdefault('extra_compile_args', []).append(token)
    return kw

def pkgconfig(*packages, **kw):
    """
    Query pkg-config for library compile and linking options. Return configuration in distutils
    Extension format.

    Usage: 

    pkgconfig('opencv')

    pkgconfig('opencv', 'libavformat')

    pkgconfig('opencv', optional='--static')

    pkgconfig('opencv', config=c)

    returns e.g.  

    {'extra_compile_args': [],
     'extra_link_args': [],
     'include_dirs': ['/usr/include/ffmpeg'],
     'libraries': ['avformat'],
     'library_dirs': []}

     Intended use:

     distutils.core.Extension('pyextension', sources=['source.cpp'], **c)

     Set PKG_CONFIG_PATH environment variable for nonstandard library locations.

    based on work of Micah Dowty (http://code.activestate.com/recipes/502261-python-distutils-pkg-config/)
    """
    config = kw.setdefault('config', {})
    optional_args = kw.setdefault('optional', '')

    # { <distutils Extension arg>: [<pkg config option>, <prefix length to strip>], ...}
    flag_map = {'include_dirs': ['--cflags-only-I', 2],
                'library_dirs': ['--libs-only-L', 2],
                'libraries': ['--libs-only-l', 2],
                'extra_compile_args': ['--cflags-only-other', 0],
                'extra_link_args': ['--libs-only-other', 0],
                }
    for package in packages:
        for distutils_key, (pkg_option, n) in flag_map.items():
            items = subprocess.check_output(['pkg-config', optional_args, pkg_option, package]).decode('utf8').split()
            config.setdefault(distutils_key, []).extend([i[n:] for i in items])
    return config

alpsinfo = Extension('alpsinfo',
                     **pkgconfig('cray-alps', 'cray-alpsutil', config={'extra_compile_args':['--std=gnu99',]}),
                     sources = ['alpsinfo.c'])

setup (name = 'AlpsInfo',
       version = '1.0',
       description = 'Query ALPS info on Cray supercomputers',
       author = 'Colin MacLean',
       author_email = 'cmaclean@illinois.edu',
       url = 'https://github.com/camaclean/alpsinfo',
       long_description = '''
Get a dictionary of ALPS info about the currently running aprun. Usage:

import alpsinfo, pprint
pprint.pprint(alpsinfo.info())
''',
       ext_modules = [alpsinfo])
