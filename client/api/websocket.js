const WebSocket = require("ws");

function initWebSocket(server) {
  const wss = new WebSocket.Server({ server });

  wss.on("connection", (ws) => {
    console.log("Cliente conectado via WebSocket");

    ws.on("close", () => {
      console.log("Cliente desconectado");
    });
  });

  return wss;
}

module.exports = initWebSocket;