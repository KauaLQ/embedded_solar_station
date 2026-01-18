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

app.get("/api/solar/range-info", async (req, res) => {
  try {
    const result = await db.query(`
      SELECT
        MIN(received_at) AS min,
        MAX(received_at) AS max
      FROM solar_data
    `);

    res.json(result.rows[0]);
  } catch (err) {
    res.status(500).json({ error: "Erro ao buscar range" });
  }
});

app.get("/api/solar/range", async (req, res) => {
  try {
    const { start, end } = req.query;

    if (!start || !end) {
      return res.status(400).json({
        error: "Parâmetros start e end são obrigatórios"
      });
    }

    const bounds = await db.query(`
      SELECT 
        MIN(received_at) AS min,
        MAX(received_at) AS max
      FROM solar_data
    `);

    const min = new Date(bounds.rows[0].min).getTime();
    const max = new Date(bounds.rows[0].max).getTime();
    const s = new Date(start).getTime();
    const e = new Date(end).getTime();

    if (s < min) {
      return res.status(400).json({
        error: "Horário inicial menor que o mínimo disponível no banco"
      });
    }

    if (e > max) {
      return res.status(400).json({
        error: "Horário final maior que o máximo disponível no banco"
      });
    }

    if (s >= e) {
      return res.status(400).json({
        error: "Horário inicial deve ser menor que o final"
      });
    }

    const result = await db.query(
      `
      SELECT *
      FROM solar_data
      WHERE received_at BETWEEN $1 AND $2
      ORDER BY received_at ASC
      `,
      [start, end]
    );

    res.json(result.rows);
  } catch (err) {
    res.status(500).json({ error: "Erro ao buscar dados filtrados" });
  }
});

app.get("/api/solar/latest", async (req, res) => {
  const limit = Number(req.query.limit) || 60;

  try {
    const result = await db.query(
      `
      SELECT *
      FROM solar_data
      ORDER BY received_at DESC
      LIMIT $1
      `,
      [limit]
    );

    // inverter para ordem cronológica
    res.json(result.rows.reverse());
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "Erro ao buscar histórico" });
  }
});

server.listen(3001, () => {
  console.log("Servidor rodando na porta 3001");
});