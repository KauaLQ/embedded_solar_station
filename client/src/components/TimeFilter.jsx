import { useEffect, useState } from "react";
import { useDashboard } from "../context/DashboardContext";

export default function TimeFilter() {
  const { setMode, setFilterData } = useDashboard();
  const [min, setMin] = useState("");
  const [max, setMax] = useState("");
  const [start, setStart] = useState("");
  const [end, setEnd] = useState("");

  // Função auxiliar para converter Date para o formato YYYY-MM-DDTHH:mm
  const formatToDateTimeLocal = (date) => {
    const d = new Date(date);
    // Ajusta para o fuso horário local
    const offset = d.getTimezoneOffset() * 60000;
    const localISOTime = new Date(d.getTime() - offset).toISOString().slice(0, 16);
    return localISOTime;
  };

  useEffect(() => {
    fetch("http://localhost:3001/api/solar/range-info")
      .then((res) => res.json())
      .then((data) => {
        // Armazenamos no formato que o input entende
        const formattedMin = formatToDateTimeLocal(data.min);
        const formattedMax = formatToDateTimeLocal(data.max);

        setMin(formattedMin);
        setMax(formattedMax);
        setStart(formattedMin);
        setEnd(formattedMax);
      })
      .catch(err => console.error("Erro ao buscar range:", err));
  }, []);

  async function applyFilter() {
    // Ao enviar para a API, você pode precisar converter de volta para ISO UTC
    const res = await fetch(
      `http://localhost:3001/api/solar/range?start=${new Date(start).toISOString()}&end=${new Date(end).toISOString()}`
    );
    const data = await res.json();

    setFilterData(data);
    setMode("filter");
  }

  function backToStreaming() {
    setFilterData(null);
    setMode("stream");
  }

  if (!min || !max) return <p>Carregando filtros...</p>;

  return (
    <div style={{ marginBottom: 20 }}>
      <input
        type="datetime-local"
        value={start}
        min={min}
        max={max}
        onChange={(e) => setStart(e.target.value)}
      />
      <input
        type="datetime-local"
        value={end}
        min={min}
        max={max}
        onChange={(e) => setEnd(e.target.value)}
      />

      <button onClick={applyFilter}>Filtrar</button>
      <button onClick={backToStreaming}>Voltar ao Streaming</button>
    </div>
  );
}