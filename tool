#!/bin/bash

function red() {
  printf "\033[31m$*\033[0m"
}

function green() {
  printf "\033[32m$*\033[0m"
}

function run() {
  ignore=$1
  if [ "$ignore" == "_ignore_" ]; then
    shift
  fi
   
  echo "$(green [run]) $*"
  "$@"
  local errcode="$?"
  if [[ "$errcode" != "0" && "$ignore" != "_ignore_" ]]; then
    red [err: $errcode] "$*\n"
    exit 1
  fi
}
