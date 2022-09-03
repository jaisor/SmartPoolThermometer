const Alexa = require('ask-sdk-core');
const mqttRestApi = require('./mqtt-rest-api.js');

function tempBriefTag(strings, seconds, temp, tempUnit, assessment) {
  const str0 = strings[0]; // "As of "
  const str1 = strings[1]; // " the pool temperature is "
  const str2 = strings[2]; // " the pool temperature is "
  const str3 = strings[3]; // ". Jumping in would be "

  const ageStr = seconds > 3600 ? 'last reading' : seconds > 60 ? Math.ceil(seconds / 60) + ' minutes ago' : seconds + ' seconds ago';
  const assessmentStr = mqttRestApi.getTemperatureAssessment(temp);

  return `${str0}${ageStr}${str1}${Math.round(temp)}${str2}${tempUnit}${str3}${assessmentStr}`;
}

const LaunchRequestHandler = {
    canHandle(handlerInput) {
      return Alexa.getRequestType(handlerInput.requestEnvelope) === 'LaunchRequest';
    },
    async handle(handlerInput) {
      const r = await mqttRestApi.getTemperatureBrief();
      console.log(r);
  
      var speechText = 'Failed to get temperature';
      if (r['temperature']) {
        speechText = tempBriefTag`As of ${r['reading_age_seconds']} the pool temperature was ${r['temperature']} degrees ${r['temperature_unit']}. Jumping in would be ${r['temperature']}`;
      }
  
      return handlerInput.responseBuilder
        .speak(speechText)
        .reprompt(speechText)
        .withSimpleCard('Pool temperature brief', speechText)
        .withShouldEndSession(true)
        .getResponse();
    }
};

const AskPoolTemperatureIntentHandler = {
    canHandle(handlerInput) {
      return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
        && Alexa.getIntentName(handlerInput.requestEnvelope) === 'AskPoolTemperatureIntent';
    },
    async handle(handlerInput) {
      const r = await getDataFromApi();
      console.log(r);
  
      var speechText = 'Failed to get temperature';
      if (r['temperature']) {
        const tF = Math.round(r['temperature'] * 1.8 + 32);
        speechText = `Pool temperature is ${tF} degrees fahrenheit`;
      }
  
      return handlerInput.responseBuilder
        .speak(speechText)
        .withSimpleCard('Hey Alex, your dad made me say this!', speechText)
        .getResponse();
    }
};

const HelpIntentHandler = {
    canHandle(handlerInput) {
      return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
        && Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.HelpIntent';
    },
    handle(handlerInput) {
      const speechText = 'You can ask me about the pool temperature!';
  
      return handlerInput.responseBuilder
        .speak(speechText)
        .reprompt(speechText)
        .withSimpleCard('You can ask me about the pool temperature!', speechText)
        .getResponse();
    }
};

const CancelAndStopIntentHandler = {
    canHandle(handlerInput) {
      return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
        && (Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.CancelIntent'
          || Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.StopIntent');
    },
    handle(handlerInput) {
      const speechText = 'Goodbye!';
  
      return handlerInput.responseBuilder
        .speak(speechText)
        .withSimpleCard('Goodbye!', speechText)
        .withShouldEndSession(true)
        .getResponse();
    }
};

const SessionEndedRequestHandler = {
    canHandle(handlerInput) {
      return Alexa.getRequestType(handlerInput.requestEnvelope) === 'SessionEndedRequest';
    },
    handle(handlerInput) {
      // Any clean-up logic goes here.
      return handlerInput.responseBuilder.getResponse();
    }
};

const ErrorHandler = {
    canHandle() {
      return true;
    },
    handle(handlerInput, error) {
        console.log(`Error handled: ${error.message}`);

        return handlerInput.responseBuilder
            .speak('Sorry, I don\'t understand your command. Please say it again.')
            .reprompt('Sorry, I don\'t understand your command. Please say it again.')
            .getResponse();
    }
};

let skill;

exports.handler = async function (event, context) {
    console.log(`REQUEST++++${JSON.stringify(event)}`);
    if (!skill) {
        skill = Alexa.SkillBuilders.custom()
            .addRequestHandlers(
                LaunchRequestHandler,
                AskPoolTemperatureIntentHandler,
                HelpIntentHandler,
                CancelAndStopIntentHandler,
                SessionEndedRequestHandler,
            )
            .addErrorHandlers(ErrorHandler)
            .create();
    }

    const response = await skill.invoke(event, context);
    console.log(`RESPONSE++++${JSON.stringify(response)}`);

    return response;
};

exports.handler = Alexa.SkillBuilders.custom()
  .addRequestHandlers(
    LaunchRequestHandler,
    AskPoolTemperatureIntentHandler,
    HelpIntentHandler,
    CancelAndStopIntentHandler,
    SessionEndedRequestHandler)
  .addErrorHandlers(ErrorHandler)
  .lambda();