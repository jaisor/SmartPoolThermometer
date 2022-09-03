FROM node:16-slim AS BUILD_IMAGE

WORKDIR /usr/src/app

COPY package*.json ./
RUN npm ci --omit=dev

COPY . .

FROM node:16-slim

ENV HTTP_PORT=3443
ENV HTTPS_PRIVATE_KEY_PATH="/var/local/server.key"
ENV HTTPS_CERTIFICATE_PATH="/var/local/server.crt"
ENV API_KEY="ChangeMeToSomethingSecure"
ENV MQTT_SERVER="mqtt://server.lan"
ENV MQTT_TOPIC="home/pool"

WORKDIR /usr/src/app

COPY --from=BUILD_IMAGE /usr/src/app/index.js ./index.js
COPY --from=BUILD_IMAGE /usr/src/app/node_modules ./node_modules

EXPOSE 8080
ENTRYPOINT  [ "node", "index.js" ]
