# ml-wifi

## API

### AP mode

``` js

var wifi = require('ml-wifi');
var Wifi = new wifi({
  mode: 'ap', // default is station
  auth: 'PSK_WPA2',
  ssid: 'Input your ssid',
  password: 'Input your password',
});

```

### Station mode

``` js
var EventEmitter = require('ml-event').EventEmitter;
var eventStatus = new EventEmitter();
global.eventStatus = eventStatus;

var wifi = require('ml-wifi');
var Wifi = new wifi({
  mode: 'station', // default is station
  auth: 'PSK_WPA2',
  ssid: 'Input your ssid',
  password: 'Input your password',
});

global.eventStatus.on('wifiConnect', function(){
  // if wifi connect ...
});

```


