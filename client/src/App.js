import { WebSocketProvider } from "./context/WebSocketContext";
import { DashboardProvider } from "./context/DashboardContext";
import { ToastContainer } from 'react-toastify';
import 'react-toastify/dist/ReactToastify.css';
import TimeFilter from "./components/TimeFilter";
import RealtimeChart from "./components/RealtimeChart";

function App() {
  return (
    <WebSocketProvider>
      <DashboardProvider>
        <TimeFilter />
          <div style={{ padding: 20 }}>
            <h1>Solar Dashboard</h1>
            <RealtimeChart
              title="Iluminância"
              variables={[
                { key: "lux1", label: "Lux 1", color: "#ff7300" },
                { key: "lux2", label: "Lux 2", color: "#387908" },
                { key: "lux3", label: "Lux 3", color: "#8884d8" }
              ]}
              windowSize={60}
              height={250}
            />
            <RealtimeChart
              title="Temperatura"
              variables={[
                { key: "temperatura", label: "Temperatura", color: "#d62728" }
              ]}
              windowSize={120}
              height={250}
            />
            <RealtimeChart
              title="Variáveis Elétricas"
              variables={[
                { key: "tensao_entrada", label: "Tensão IN", color: "#ff7300" },
                { key: "tensao_shunt", label: "Tensão SHUNT", color: "#387908" },
                { key: "corrente", label: "Corrente", color: "#8884d8" },
                { key: "potencia", label: "Potência", color: "#1f77b4" }
              ]}
              windowSize={300}
              height={250}
            />
            <RealtimeChart
              title="Angulo do Painel"
              variables={[
                { key: "pitch", label: "Pitch", color: "#ff7300" },
                { key: "roll", label: "Roll", color: "#387908" }
              ]}
              windowSize={60}
              height={250}
            />

            <ToastContainer position="top-right" autoClose={3000} />
          </div>
      </DashboardProvider>
    </WebSocketProvider>
  );
}

export default App;