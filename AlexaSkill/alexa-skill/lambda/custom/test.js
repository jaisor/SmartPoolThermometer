const mqttRestApi = require('./mqtt-rest-api.js');

mqttRestApi.getDeviceStatus().then((result) => {
  console.log(JSON.stringify(result));
})
.catch((err) => {
  console.log(`Error: ${err}`);
});

// Brief

function tempBriefTag(strings, seconds, temp, tempUnit, assessment) {
  const str0 = strings[0]; // "As of "
  const str1 = strings[1]; // " the pool temperature is "
  const str2 = strings[2]; // " the pool temperature is "
  const str3 = strings[3]; // ". Jumping in would be "

  const ageStr = seconds > 3600 ? 'last reading' : seconds > 60 ? Math.ceil(seconds / 60) + ' minutes ago' : seconds + ' seconds ago';
  const assessmentStr = mqttRestApi.getTemperatureAssessment(temp);

  return `${str0}${ageStr}${str1}${Math.round(temp)}${str2}${tempUnit}${str3}${assessmentStr}`;
}

mqttRestApi.getTemperatureBrief().then((r) => {
  console.log(tempBriefTag`As of ${r['reading_age_seconds']} the pool temperature was ${r['temperature']} degrees ${r['temperature_unit']}. Jumping in would be ${r['temperature']}`);
})
.catch((err) => {
  console.log(`Error: ${err}`);
});
