#!/bin/bash -e

PROJECT_PATH="$PWD"
MOSQUITTO_IMAGE="${MOSQUITTO_IMAGE:=eclipse-mosquitto}"
HOME_ASSISTANT_IMAGE="${HOME_ASSISTANT_IMAGE:=homeassistant/raspberrypi4-homeassistant:stable}"
PROXY_IMAGE="${PROXY_IMAGE:=weather-station/proxy-image}"
SOURCE="Gateway"
CONTAINER_CMD=docker

function container_exec {
    $CONTAINER_CMD exec -ti "$1" /bin/bash -c "$2"
}

function clear_old_containers {
    echo "Clearing old containers..."
    $CONTAINER_CMD stop "proxy" || true
    $CONTAINER_CMD stop "home-assistant" || true
    $CONTAINER_CMD stop "mosquitto" || true
}

clear_old_containers

echo "Starting mosquitto container..."
MQTT_ID="$($CONTAINER_CMD run -d --rm --user "$(id -u)":"$(id -g)" --name="mosquitto" -v $PROJECT_PATH/$SOURCE/mosquitto/conf/mosquitto.conf:/mosquitto/config/mosquitto.conf -v $PROJECT_PATH/$SOURCE/mosquitto/log:/mosquitto/log $MOSQUITTO_IMAGE)"
echo "Sleep for 1 sec to give mosquitto server chance to properly start"
sleep 1

echo "Starting home-assistant container..."
HOME_ASSISTANT_ID="$($CONTAINER_CMD run --init -d --rm --user "$(id -u)":"$(id -g)" --name="home-assistant" --net=container:$MQTT_ID -v $PROJECT_PATH/$SOURCE/home-assistant/conf:/config $HOME_ASSISTANT_IMAGE)"
echo "Sleep for 10 secs to give home assistant chance to properly start"
sleep 10

echo "Starting proxy container..."
PROXY_ID="$($CONTAINER_CMD run -d --rm --name="proxy" --net=container:$MQTT_ID --device /dev/spidev0.0 --device /dev/gpiochip0 -v $PROJECT_PATH/$SOURCE/proxy/conf:/proxy/conf -v $PROJECT_PATH/$SOURCE/proxy/log:/proxy/log $PROXY_IMAGE)"
