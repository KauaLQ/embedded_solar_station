import { useEffect, useState } from "react";
import { useDashboard } from "../../context/DashboardContext";
import { toast } from "react-toastify";
import './TimeFilter.css';

export default function TimeFilter() {
  const {mode, setMode, setFilterData } = useDashboard();
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
      .catch(err => {
        console.error("Erro ao buscar range:", err);
        toast.error("Erro ao carregar limites de data"); // Erro no carregamento
      });
  }, []);

  function isOutOfRange(start, end, min, max) {
    const s = new Date(start).getTime();
    const e = new Date(end).getTime();
    const minT = new Date(min).getTime();
    const maxT = new Date(max).getTime();

    if (s < minT) return "O horário inicial é menor que o mínimo disponível.";
    if (e > maxT) return "O horário final é maior que o máximo disponível.";
    if (s >= e) return "O horário inicial deve ser menor que o final.";

    return null;
  }

  async function applyFilter() {
    const error = isOutOfRange(start, end, min, max);
    
    if (error) {
        toast.warn(error);
        return;
    }

    try {
        const res = await fetch(
          `http://localhost:3001/api/solar/range?start=${new Date(start).toISOString()}&end=${new Date(end).toISOString()}`
        );

        if (!res.ok) {
            const err = await res.json();
            toast.error(err.error || "Erro ao aplicar filtro"); // 3. Erro da API
            return;
        }

        const data = await res.json();
        setFilterData(data);
        setMode("filter");
        toast.success("Filtro aplicado com sucesso!"); // 4. Feedback de sucesso
    } catch (err) {
        toast.error("Falha na conexão com o servidor");
    }
  }

  function backToStreaming() {
    setFilterData(null);
    setMode("stream");
    toast.info("Voltando para o modo de streaming");
  }

  if (!min || !max) return <p>Carregando filtros...</p>;

  return (
    <div id="container-timefilter">
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

      <button className="mybutton" onClick={applyFilter}><i className="fa-solid fa-magnifying-glass-chart"></i> Filtrar</button>
      <button className={`mybutton ${mode === 'filter' ? 'disabled-stream' : ''}`} onClick={backToStreaming}><i className="fa-solid fa-square-rss"></i> Ao vivo</button>
    </div>
  );
}