const Alexa = require('ask-sdk-core');
const https = require('https');

const config = {
  apoiHost: process.env.API_HOST,
  apiPort: process.env.API_PORT,
  apiPath: process.env.API_PATH,
  apiKey: process.env.API_KEY
};

process.env["NODE_TLS_REJECT_UNAUTHORIZED"] = 0;

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


const LaunchRequestHandler = {
    canHandle(handlerInput) {
      return Alexa.getRequestType(handlerInput.requestEnvelope) === 'LaunchRequest';
    },
    async handle(handlerInput) {
      const r = await getDataFromApi();
      console.log(r);
  
      var speechText = 'Failed to get temperature';
      if (r['temp_c']) {
        const tF = Math.round(r['temp_c'] * 1.8 + 32);
        speechText = `Pool temperature is ${tF} degrees Fahrenheit`;
      }
  
      return handlerInput.responseBuilder
        .speak(speechText)
        .reprompt(speechText)
        .withSimpleCard('Welcome to the Pool Thermometer skill. Ask me what the pool temperature is!', speechText)
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