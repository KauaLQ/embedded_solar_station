const express = require("express");
const http = require("http");
const cors = require("cors");

const db = require("./db");
const initWebSocket = require("./websocket");

const app = express();
app.use(cors());

const server = http.createServer(app);
const wss = initWebSocket(server);

// LISTEN no canal do Postgres
db.query("LISTEN solar_data_channel");

db.on("notification", (msg) => {
  const data = JSON.parse(msg.payload);

  // envia para todos os clientes conectados
  wss.clients.forEach((client) => {
    if (client.readyState === 1) {
      client.send(JSON.stringify(data));
    }
  });
});

server.listen(3001, () => {
  console.log("Servidor rodando na porta 3001");
});
