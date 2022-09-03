const https = require('https');

const config = {
  apoiHost: process.env.API_HOST,
  apiPort: process.env.API_PORT,
  apiPath: process.env.API_PATH,
  apiKey: process.env.API_KEY
};

process.env['NODE_TLS_REJECT_UNAUTHORIZED'] = 0;

function getDataFromApi() {
  return new Promise(((resolve, reject) => {
    var options = {
      host: config.apoiHost, 
      port: config.apiPort,
      path: config.apiPath,
      method: 'GET',
      rejectUnauthorized: false,
      requestCert: true,
      agent: false,
      headers: {
        'api_key': config.apiKey
      }
    };
    
    const request = https.request(options, (response) => {
      response.setEncoding('utf8');
      let returnData = '';

      response.on('data', (chunk) => {
        returnData += chunk;
      });

      response.on('end', () => {
        resolve(JSON.parse(returnData));
      });

      response.on('error', (error) => {
        reject(error);
      });
    });
    request.end();
  }));
}

function getTemperatureBrief() {
  return getDataFromApi().then(r => {
    return {
      'temperature': r['temperature'],
      'temperature_unit': r['temperature_unit'],
      'reading_age_seconds': Math.ceil(Math.abs(Date.now() - Date.parse(r['timestamp_iso8601'])) / 1000),
    };
  })
}

module.exports = {
  getTemperatureBrief: function() {
    return getDataFromApi().then(r => {
      return {
        'temperature': r['temperature'],
        'temperature_unit': r['temperature_unit'],
        'reading_age_seconds': Math.ceil(Math.abs(Date.now() - Date.parse(r['timestamp_iso8601'])) / 1000),
      };
    })
  },
  getDeviceStatus: function() {
    return getDataFromApi().then(r => {
      return {
        'temperature': r['temperature'],
        'temperature_unit': r['temperature_unit'],
        'reading_age_seconds': Math.ceil(Math.abs(Date.now() - Date.parse(r['timestamp_iso8601'])) / 1000),
        //
        'battery_volts': r['battery_v'],
        'uptime_millis': r['uptime_millis'],
        'wifi_percent': r['wifi_percent']
      };
    })
  },
  getTemperatureAssessment: function(t) {
    if (t<72) { return 'freezing'; }
    else if (t<78) { return 'chilly'; }
    else if (t<80) { return 'rfreshing'; }
    else if (t<85) { return 'pleasant'; }
    else if (t<90) { return 'too warm'; }
    else { return 'boiling'; }
  }
};