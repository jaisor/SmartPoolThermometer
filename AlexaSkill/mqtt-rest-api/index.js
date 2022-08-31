const fs = require('fs');
const https = require('https');

const config = {
  httpPort: process.env.HTTP_PORT,
  privateKey: process.env.HTTPS_PRIVATE_KEY_PATH,
  certificate: process.env.HTTPS_CERTIFICATE_PATH,
  apiKey: process.env.API_KEY,
  mqttServer: process.env.MQTT_SERVER,
  mqttTopic: process.env.MQTT_TOPIC
};

if (!config.mqttServer) {
  console.log(`Blank MQTT server`);
  return;
}

if (!config.mqttTopic) {
  console.log(`Blank MQTT topic`);
  return;
}

const privateKey  = fs.readFileSync(config.privateKey, 'utf8');
const certificate = fs.readFileSync(config.certificate, 'utf8');

const mqtt = require('mqtt');
const client = mqtt.connect(config.mqttServer);

var json = {};

client.on('connect', function () {
  console.log(`Connected to MQTT server ${config.mqttServer}`);
  client.subscribe(config.mqttTopic, function (err) {
    if (!err) {
      console.log(`Subscribed to MQTT topic ${config.mqttTopic}`);
    }
  })
});

client.on('message', function (topic, message) {
  console.log(`Received message from MQTT topic ${topic}`);
  json = JSON.parse(message.toString());
  console.log(JSON.stringify(json));
});

const express = require('express');
const app = express();
const port = parseInt(config.httpPort, 10);

app.get('/', (req, res) => {
  if (req.headers.api_key !== config.apiKey) {
    res.status(403).send();
    return;
  }
  res.json(json);
});


const credentials = {key: privateKey, cert: certificate};
const httpsServer = https.createServer(credentials, app);

httpsServer.listen(port, () => {
  console.log(`SmartPoolThermometer MQT API listening on port ${port}`);
});