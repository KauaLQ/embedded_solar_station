import { useEffect, useState } from "react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend
} from "recharts";

const MAX_POINTS = 60;

function App() {
  const [chartData, setChartData] = useState([]);

  useEffect(() => {
    const socket = new WebSocket("ws://localhost:3001");

    socket.onmessage = (event) => {
      const data = JSON.parse(event.data);

      const point = {
        time: new Date(data.received_at).toLocaleTimeString(),
        lux1: data.lux1,
        lux2: data.lux2,
        lux3: data.lux3
      };

      setChartData((prev) => {
        const updated = [...prev, point];
        return updated.slice(-MAX_POINTS);
      });
    };

    return () => socket.close();
  }, []);

  return (
    <div style={{ padding: 20 }}>
      <h1>Solar Dashboard</h1>

      <LineChart
        width={900}
        height={400}
        data={chartData}
      >
        <CartesianGrid strokeDasharray="3 3" />
        <XAxis dataKey="time" />
        <YAxis />
        <Tooltip />
        <Legend />

        <Line
          type="monotone"
          dataKey="lux1"
          stroke="#ff7300"
          dot={false}
        />
        <Line
          type="monotone"
          dataKey="lux2"
          stroke="#387908"
          dot={false}
        />
        <Line
          type="monotone"
          dataKey="lux3"
          stroke="#8884d8"
          dot={false}
        />
      </LineChart>
    </div>
  );
}

export default App;