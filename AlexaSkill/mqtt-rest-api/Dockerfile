FROM node:16

WORKDIR /usr/src/app

COPY package*.json ./
RUN npm install

COPY index.js ./

ENV HTTP_PORT=3443
ENV HTTPS_PRIVATE_KEY_PATH=/var/local/server.key
ENV HTTPS_CERTIFICATE_PATH=/var/local/server.crt
ENV API_KEY=ChangeMeToSomethingSecure

EXPOSE 8080
ENTRYPOINT  [ "node", "index.js" ]