#!/usr/bin/env python
from setuptools import setup, find_packages

setup(
    name='disposable-redis',
    version='0.2',
    description='',
    author='EverythingMe',
    author_email='omrib@everything.me',
    url='http://github.com/EverythingMe/python-disposable-redis',
    packages=find_packages(),
    install_requires=['redis'],

    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: BSD License',
        'Programming Language :: Python :: 2.7',
        'Topic :: Database',
        'Topic :: Software Development :: Testing'
    ]
)

