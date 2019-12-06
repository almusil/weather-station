#!/bin/bash -e

PROJECT_PATH="$PWD"
MOSQUITTO_IMAGE="${MOSQUITTO_IMAGE:=eclipse-mosquitto}"
HOME_ASSISTANT_IMAGE="${HOME_ASSISTANT_IMAGE:=homeassistant/raspberrypi4-homeassistant:stable}"
SOURCE="Gateway"
CONTAINER_CMD=docker

function container_exec {
    $CONTAINER_CMD exec -ti "$1" /bin/bash -c "$2"
}

function clear_old_containers {
    echo "Clearing old containers..."
    $CONTAINER_CMD stop "home-assistant"
    $CONTAINER_CMD stop "mosquitto"
}

clear_old_containers

echo "Starting mosquitto container..."
MQTT_ID="$($CONTAINER_CMD run -d --rm --user "$(id -u)":"$(id -g)" --name="mosquitto" -v $PROJECT_PATH/$SOURCE/mosquitto/conf/mosquitto.conf:/mosquitto/config/mosquitto.conf -v $PROJECT_PATH/$SOURCE/mosquitto/log:/mosquitto/log $MOSQUITTO_IMAGE)"


echo "Starting home-assistant container..."
HOME_ASSISTANT_ID="$($CONTAINER_CMD run --init -d --rm --user "$(id -u)":"$(id -g)" --name="home-assistant" --net=container:$MQTT_ID -v $PROJECT_PATH/$SOURCE/home-assistant/conf:/config $HOME_ASSISTANT_IMAGE)"
