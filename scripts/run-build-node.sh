#!/bin/bash -e

PROJECT=${PROJECT:-${PWD##*/}}
PROJECT_PATH="$PWD"
CONTAINER_WORKSPACE="/workspace/$PROJECT"
CONTAINER_IMAGE="${CONTAINER_IMAGE:=weather-station/node-build-image}"
SOURCE="Node"
CONTAINER_CMD=docker

function run_exit {
    remove_container
}

function remove_container {
    res=$?
    [ "$res" -ne 0 ] && echo "*** ERROR: $res ***"
    $CONTAINER_CMD rm -f "$CONTAINER_ID"
}

function container_exec {
    $CONTAINER_CMD exec -ti "$CONTAINER_ID" /bin/bash -c "$1"
}

CONTAINER_ID="$($CONTAINER_CMD run --user "$(id -u)":"$(id -g)" -d -w $CONTAINER_WORKSPACE/$SOURCE -v $PROJECT_PATH:$CONTAINER_WORKSPACE:Z $CONTAINER_IMAGE)"
trap run_exit EXIT

if [ "$1" == "--shell" ];then
    container_exec "bash"
    exit 0
fi

container_exec "make clean all -C node"
