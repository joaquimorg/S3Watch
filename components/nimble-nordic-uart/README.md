# NimBLE Nordic UART driver for ESP-IDF V5.x

This library implements the Nordic UART (Universal Asynchronous Receiver-Transmitter) service over BLE (Bluetooth Low Energy) using the ESP-IDF framework. It's confirmed to work with ESP-IDF v5.1.
The library allows for easy communication between an ESP32 device and a BLE client, utilizing the Nordic UART service for a simple, serial-like communication channel.

## Features
- Implements the Nordic UART Service (NUS) for BLE communication.
- Provides a simple API for sending and receiving data over BLE.
- Supports ESP-IDF v5.1.

## Nordic UART Functions

### `nordic_uart_start`
Initializes and starts the Nordic UART service.
- `device_name`: The name of the BLE device to be advertised.
- `callback`: Function pointer to a callback function that is called on connection status changes (connected/disconnected).

### `nordic_uart_stop`
Stops the Nordic UART service and cleans up resources.

### `nordic_uart_send`
Sends a message over the Nordic UART.
- `message`: String message to be sent.

### `nordic_uart_sendln`
Sends a message followed by a newline character over the Nordic UART.
- `message`: String message to be sent.

### `nordic_uart_yield`
Allows setting a custom callback for handling received UART data.
- `uart_receive_callback`: Callback function that handles received data.

## Install to your project
To add this component to your ESP-IDF project, run:

```
idf.py add-dependency "masuidrive/nimble-nordic-uart"
```

## Getting Started

If you want to try out this library, start by cloning the repository from GitHub:

```bash
git clone https://github.com/masuidrive/esp-nimble-nordic-uart
```

Once you have cloned the repository, you can delve into the Example and Unit Test to explore further

### Run Example

To run the provided example:

```bash
cd example
idf.py build flash monitor -p /dev/cu.usbserial-*
```

## Run Unit Test on Target Device
To run the unit tests on your target device:

```bash
cd test-runner
idf.py build flash monitor -p /dev/cu.usbserial-*
```

## Connection Testing with WebBLE

You can test the connection using Chrome's WebBLE, which allows for BLE interactions from the browser. To do this, run the following command and then open http://localhost:8000 in your browser:

```bash
cd web
python -m http.server 8000 
```

This will start a simple HTTP server to host the WebBLE test interface, making it easy to connect and communicate with your ESP32 device from a web browser.

## Contributing

Contributions to this project are welcome. Please ensure that your code adheres to the existing coding style and includes appropriate tests.

## License

This project is licensed under the Apache-2.0 License.
Copyright 2021- Yuichiro Masui <masui@masuidrive.jp>
