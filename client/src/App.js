import { WebSocketProvider } from "./context/WebSocketContext";
import { DashboardProvider } from "./context/DashboardContext";
import { ToastContainer } from 'react-toastify';
import 'react-toastify/dist/ReactToastify.css';
import RealtimeChart from "./components/RealtimeChart/RealtimeChart";
import Navbar from "./components/Navbar/Navbar";
import Sidebar from "./components/Sidebar/Sidebar"
import './App.css';

function App() {
  return (
    <WebSocketProvider>
      <DashboardProvider>
          <div id="bodyApp">
            <Sidebar/>

            <div id="mainApp">
              <Navbar/>
              <h1>Overview</h1>
              <RealtimeChart
                title="ILUMINÂNCIA"
                variables={[
                  { key: "lux1", label: "Lux 1", color: "#ff7300" },
                  { key: "lux2", label: "Lux 2", color: "#387908" },
                  { key: "lux3", label: "Lux 3", color: "#8884d8" }
                ]}
                windowSize={60}
                height={250}
              />
              <RealtimeChart
                title="TEMPERATURA"
                variables={[
                  { key: "temperatura", label: "Temperatura", color: "#d62728" }
                ]}
                windowSize={120}
                height={250}
              />
              <RealtimeChart
                title="VARIÁVEIS ELÉTRICAS"
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
                title="ÂNGULO DO PAINEL"
                variables={[
                  { key: "pitch", label: "Pitch", color: "#ff7300" },
                  { key: "roll", label: "Roll", color: "#387908" }
                ]}
                windowSize={60}
                height={250}
              />

              <ToastContainer position="top-right" autoClose={3000} />
            </div>
          </div>
      </DashboardProvider>
    </WebSocketProvider>
  );
}

export default App;