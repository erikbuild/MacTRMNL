# TRMNAPPL Proxy Server

## Installation

The only requirement is Ruby 3. No gems are used.

## Usage

```sh
# get the key from https://usetrmnl.com/devices/current/edit
export ACCESS_TOKEN="my-device-api-key"

./trmnappl.rb [port]
```

## Testing

Requires netcat (`brew install netcat` on macOS).

```sh
nc localhost 31337 > trmnl.bmp
```

