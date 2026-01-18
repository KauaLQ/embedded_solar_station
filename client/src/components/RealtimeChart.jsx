import { useEffect, useState } from "react";
import { useRealtimeData } from "../context/WebSocketContext";
import { useDashboard } from "../context/DashboardContext";
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend
} from "recharts";

export default function RealtimeChart({
  title,
  variables,
  windowSize = 60
}) {
  const lastData = useRealtimeData();
  const { mode, filterData } = useDashboard();
  const [data, setData] = useState([]);

  // MODO FILTRO
  useEffect(() => {
    if (mode !== "filter" || !filterData) return;

    const formatted = filterData.map((row) => {
      const point = {
        time: new Date(row.received_at).toLocaleTimeString()
      };

      variables.forEach((v) => {
        point[v.key] = row[v.key];
      });

      return point;
    });

    setData(formatted);
  }, [mode, filterData, variables]);

  // MODO STREAMING
  // Buscar histórico inicial
  useEffect(() => {
    if (mode !== "stream") return;

    async function fetchLatest() {
      try {
        const res = await fetch(
          `http://localhost:3001/api/solar/latest?limit=${windowSize}`
        );
        const rows = await res.json();

        const formatted = rows.map((row) => {
          const point = {
            time: new Date(row.received_at).toLocaleTimeString()
          };

          variables.forEach((v) => {
            point[v.key] = row[v.key];
          });

          return point;
        });

        setData(formatted);
      } catch (err) {
        console.error("Erro ao buscar histórico:", err);
      }
    }

    fetchLatest();
  }, [mode, variables, windowSize]);

  // Receber dados em tempo real
  useEffect(() => {
    if (mode !== "stream" || !lastData) return;

    const point = {
      time: new Date(lastData.received_at).toLocaleTimeString()
    };

    variables.forEach((v) => {
      point[v.key] = lastData[v.key];
    });

    setData((prev) => {
      const updated = [...prev, point];
      return updated.slice(-windowSize);
    });
  }, [lastData, mode, variables, windowSize]);

  return (
    <>
      <h2>{title}</h2>
      <LineChart width={900} height={400} data={data}>
        <CartesianGrid strokeDasharray="3 3" />
        <XAxis dataKey="time" />
        <YAxis />
        <Tooltip />
        <Legend />
        {variables.map((v) => (
          <Line key={v.key} dataKey={v.key} stroke={v.color} dot={false} />
        ))}
      </LineChart>
    </>
  );
}

/*

async function fetchLatest() {
    const res = await fetch(
      `http://localhost:3001/api/solar/latest?limit=${windowSize}`
    );
    const rows = await res.json();

    const formatted = rows.map((row) => {
      const point = {
        time: formatTime(row.received_at)
      };

      variables.forEach((v) => {
        point[v.key] = row[v.key];
      });

      return point;
    });

    setData(formatted);
  }

  fetchLatest();
}, [mode, windowSize, variables]);

*/