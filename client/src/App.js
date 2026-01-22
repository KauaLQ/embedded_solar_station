import { WebSocketProvider } from "./context/WebSocketContext";
import { DashboardProvider } from "./context/DashboardContext";
import { ToastContainer } from 'react-toastify';
import RealtimeChart from "./components/RealtimeChart/RealtimeChart";
import Sidebar from "./components/Sidebar/Sidebar";
import Navbar from "./components/Navbar/Navbar";
import 'react-toastify/dist/ReactToastify.css';
import './App.css';

function App() {
  return (
    <WebSocketProvider>
      <DashboardProvider>
        <div id="bodyApp">
          <Sidebar />
          <div id="mainApp">
            <Navbar />
            <div id="page_section">
              <div id="page_infos">
                <span className="page-description">
                  Visão Geral
                </span>
                <span className="page-description">
                  Veja os gráficos com os dados do painel fotovoltáico
                </span>
              </div>
              <div id="page_section_buttons">
                <button className="mybutton page-section"><i class="fa-solid fa-share-nodes"></i> Compartilhar</button>
                <button className="mybutton page-section"><i class="fa-solid fa-file-csv"></i> Exportar</button>
              </div>
            </div>

            <RealtimeChart
              title="Iluminância (LUX)"
              subtitle="Dados de iluminância dos 3 sensores BH1750"
              variables={[
                { key: "lux1", label: "BH1750-1", color: "#ff7300" },
                { key: "lux2", label: "BH1750-2", color: "#387908" },
                { key: "lux3", label: "BH1750-3", color: "#8884d8" }
              ]}
              windowSize={60}
              height={250}
            />
            <RealtimeChart
              title="Temperatura (°C)"
              subtitle="Temperatura do módulo fotovoltaico"
              variables={[
                { key: "temperatura", label: "Temperatura", color: "#d62728" }
              ]}
              windowSize={120}
              height={250}
            />
            <RealtimeChart
              title="Variáveis Elétricas"
              subtitle="Tensão de entrada (V), Tensão do sensor (V), Corrente (A) e Potência elétrica (W)"
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
              title="Ângulação do Painel (Graus)"
              subtitle="Posicionamento do painel solar em relação ao eixo de giro"
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