function wifi(config) {
  __wifi({
    mode: config.mode,
    auth: config.auth,
    ssid: config.ssid,
    password: config.password,
  });
}
wifi.prototype.connect = function(cb) {
  return global.eventStatus.on('wifiConnect', cb);
}
module.exports = wifi;