const { Client } = require("pg");

const db = new Client({
  host: "localhost",
  user: "postgres",
  password: "170121Kh",
  database: "solar_station",
  port: 5432,
});

db.connect();

module.exports = db;
