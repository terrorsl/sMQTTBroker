# sMQTTBroker

Simple MQTT Broker

sMQTTBroker is light and fast mqtt broker

## Stable release

[![Release](https://img.shields.io/github/v/release/terrorsl/sMQTTBroker)](https://github.com/terrorsl/sMQTTBroker/releases/latest)
![GitHub Release Date](https://img.shields.io/github/release-date/terrorsl/sMQTTBroker)
![GitHub commits since latest release (by date) for a branch](https://img.shields.io/github/commits-since/terrorsl/sMQTTBroker/latest)
![GitHub release (latest by date)](https://img.shields.io/github/downloads/terrorsl/sMQTTBroker/latest/total)
[![Check Arduino](https://github.com/terrorsl/sMQTTBroker/actions/workflows/checkarduino.yml/badge.svg?branch=main)](https://github.com/terrorsl/sMQTTBroker/actions/workflows/checkarduino.yml)

## Platform:
[![Esp8266](https://img.shields.io/badge/platform-ESP8266-green)](https://www.espressif.com/en/products/socs/esp8266)
[![Esp32](https://img.shields.io/badge/platform-ESP32-green)](https://www.espressif.com/en/products/socs/esp32)

## Documentation:
[![Mqtt 3.1.1](https://img.shields.io/badge/Mqtt-%203.1.1-yellow)](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/errata01/os/mqtt-v3.1.1-errata01-os-complete.html#_Toc442180822)

## IDE

[![arduino-library-badge](https://www.ardu-badge.com/badge/sMQTTBroker.svg?)](https://www.ardu-badge.com/sMQTTBroker)

[![PlatformIO Registry](https://badges.registry.platformio.org/packages/terrorsl/library/sMQTTBroker.svg)](https://registry.platformio.org/libraries/terrorsl/sMQTTBroker)
## Features

- Mqtt 3.1.1 / Qos 0, 1 supported

## Quickstart
<!-- * install [sMQTTBroker library](https://github.com/terrorsl/sMQTTBroker)
  (you can use the Arduino library manager and search for sMQTTBroker)
* ~~make sMQTTConfig.h~~ -->

```c++
#include<sMQTTBroker.h>
```
```c++
sMQTTBroker broker;
#define PORT 1883
```
```c++
void setup(){
  broker.init(PORT);
};
```
```c++
void loop(){
  broker.update();
};
```

## Examples
[SimpleBroker](https://github.com/terrorsl/sMQTTBroker/examples/simplebroker)

[AdvanceBroker](https://github.com/terrorsl/sMQTTBroker/examples/simplebroker)

## TODO

* [x] Client Identifier
* [ ] DUP
* [x] keep alive
* [x] user/password
* [x] append support Qos 1
* [x] PUBACK
* [ ] PUBREC
* [ ] PUBREL
* [ ] PUBCOMP
* [ ] append support MQTT 5.0

<a href="https://www.buymeacoffee.com/terror85a"><img src="https://img.buymeacoffee.com/button-api/?text=Buy me a coffee&emoji=&slug=terror85a&button_colour=FFDD00&font_colour=000000&font_family=Cookie&outline_colour=000000&coffee_colour=ffffff" /></a>

## License
[![GitHub](https://img.shields.io/github/license/terrorsl/sMQTTBroker)](https://github.com/terrorsl/sMQTTBroker/blob/main/LICENSE)
