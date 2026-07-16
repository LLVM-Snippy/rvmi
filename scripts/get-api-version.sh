#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")"

grep --perl-regexp --only-matching \
  "(?<=RVMAPI_CURRENT_INTERFACE_VERSION )([[:digit:]]+)" \
  ../include/RISCVModel/RVM.h
