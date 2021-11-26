#!/bin/bash

function red() {
  printf "\033[31m%s\033[0m" "$@"
}

function green() {
  printf "\033[32m%s\033[0m" "$@"
}

function run() {
  echo "$(green [run]) $@"
  "$@"
  local errcode="$?"
  if [ "$errcode" != "0" ]; then
    echo "$(red [err: $errcode]) $@"
    exit 1
  fi 
}
