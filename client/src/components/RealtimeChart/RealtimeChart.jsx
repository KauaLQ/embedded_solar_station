import { useEffect, useState } from "react";
import { useRealtimeData } from "../../context/WebSocketContext";
import { useDashboard } from "../../context/DashboardContext";
import {
  LineChart, ResponsiveContainer, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend
} from "recharts";
import './RealtimeChart.css';

export default function RealtimeChart({
  title,
  subtitle,
  variables,
  windowSize = 60,
  // width = 900, // Não usada por enquanto
  height = 400
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
    <div id="chart_div">
      <div id="chart_infos">
        <span className="chart-description">
          <b>{title}</b>
        </span>
        <span className="chart-description">
          {subtitle}
        </span>
      </div>

      <div id="chart_charts" style={{ width: '100%', height: height }}>
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={data} margin={{ top: 5, right: 20, left: -30, bottom: 5 }}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="time"/>
            <YAxis />
            <Tooltip />
            <Legend />
            {variables.map((v) => (
              <Line
                key={v.key}
                dataKey={v.key}
                name={v.label}
                stroke={v.color}
                dot={false}
                isAnimationActive={false}
              />
            ))}
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}