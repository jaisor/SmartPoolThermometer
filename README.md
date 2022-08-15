# Smart Pool Thermometer

## Requirements

* Floating water-tight pool sensor
* Temperature and other telemetry (internal battery voltage) readings submitted over Wifi via MQTT at reasonable intervals (~5min)
* Low power consumption, battery powered with solar recharge during the day 
* Sensor initial configuration via self-hosted AP (`ESP8266ST_XXXXX` / `password123`) / IP (`192.168.4.1`)
* Sensor resettable to defaults by 3 power-cycles within 2 seconds of boot
* Data visualization - MQTT - Prpmetheus - Grafana dashboard

## Notes

Code is compatible with both ESP8266 and ESP32 boards, but ESP8266 draws signifficantly less power (4mA) during deep sleep

## Sensor

### ESP8266 / ESP32 Development

USB driver for Windows required: [Silicon Labs CP210X](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)

Recommended: [VSCode](https://code.visualstudio.com/) + [PlatformIO](https://platformio.org/)

### Bill of materials (BOM)

* [ESP8266 ESP-12](https://www.amazon.com/gp/product/B081PX9YFV)
* [DS18B20 Temperature Sensor Waterproof](https://www.amazon.com/dp/B012C597T0)
* [JST 3 Pin Connector](https://www.amazon.com/dp/B01DUC1PW6)
* [2x 10K Resistors; 1x 1K Resistor](https://www.amazon.com/dp/B08FD1XVL6)
* [6x 5V 60mA Epoxy Solar Panel](https://www.amazon.com/dp/B0736W4HK1)
* [18650 Battery Clip Holder](https://www.amazon.com/dp/B0721Y3NDQ)
* [JST Connectors](https://www.amazon.com/dp/B071XN7C43)
* [TP4056 Battery Charger Module](https://www.amazon.com/dp/B098989NRZ)
* [Prototype board (optional)](https://www.amazon.com/dp/B00FXHXT80)

### Schematic 

![Schematic](/img/schematic.png)

### Enclosure 

## Data visualization

Useful articles and guides:
* https://frederic-hemberger.de/notes/prometheus/monitoring-temperature-and-humidity-with-mqtt/
* https://grafana.com/docs/grafana/latest/setup-grafana/configure-docker/
* https://grafana.com/docs/grafana-cloud/quickstart/docker-compose-linux/

The below instructions are better suited for Desktop (local) install. Server deployments should probably use a docker-compose, dedicated users, security, auth, backup and scaling considerations. 

### MQTT broker

```
```

```
docker run -dit \
    --name=mqtt-mac \
    --restart=unless-stopped \
    -p 1883:1883 \
    -v "$APPROPRIATE_VAR_PATH/mosquitto:/mosquitto" \
    eclipse-mosquitto:latest
```

### MQTT exporter

```
docker run -dit \
  -v "$APPROPRIATE_VAR_PATH/prometheus/mqtt_exporter.yaml:/conf/mqtt_exporter.yaml:ro" \
  -p "9344:9344" --name mqtt_exporter \
  fhemberger/mqtt_exporter:latest -c /conf/mqtt_exporter.yaml
```

### Prometehus

`prometheus/config.yml`

```
scrape_configs:

# Prometehus itself
  - job_name: 'prometheus'
    scrape_interval: 30s
    static_configs:
      - targets: ['localhost:9090']

# MQTT exporter
  - job_name: 'mqtt'
    scrape_interval: 30s
    static_configs:
    - targets: ['<HOSTNAME>.local:9344']
```

```
docker run -dit --restart unless-stopped -p 9090:9090 \
    --name prometheus-mac \
    -v "$APPROPRIATE_VAR_PATH/prometheus:/data" \
    prom/prometheus:latest \
    --config.file="/data/config.yml" \
    --storage.tsdb.path="/data/prometheus"
```

### Grafana

```
docker run -dit --restart unless-stopped -p "3000:3000" --name grafana-mac grafana/grafana:latest
```

### Dashboard

