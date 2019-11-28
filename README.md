# weather-station

## Node
### Bulding build container
Build node building container, under project root directory: 
```
docker build --rm \
 -t weather-station/node-build-image \
 -f ./docker/Dockerfile.node \
 .
 ```

New container is needed in case you want to update libopencm3 to the latest
or bump ARM GCC version.

### Running build
Build node source, under project root directory: 
```
./scripts/run-build-node.sh
```

Open the container shell without executing the build run: 
```
./scripts/run-build-node.sh --shell
```

## Schematic and board
Schematic and board design are under HW directory
