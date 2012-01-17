#!/bin/bash

touch NEWS AUTHORS ChangeLog
autoreconf -i && \
./configure "$@"
