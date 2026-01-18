import { createContext, useContext, useEffect, useState } from "react";

const WebSocketContext = createContext(null);

export function WebSocketProvider({ children }) {
  const [lastMessage, setLastMessage] = useState(null);

  useEffect(() => {
    const socket = new WebSocket("ws://localhost:3001");

    socket.onopen = () => {
      console.log("[CONTEXT] WebSocket global conectado");
    };

    socket.onmessage = (event) => {
      const data = JSON.parse(event.data);
      setLastMessage(data);
    };

    socket.onerror = (err) => {
      console.error("[CONTEXT] WebSocket erro:", err);
    };

    return () => socket.close();
  }, []);

  return (
    <WebSocketContext.Provider value={lastMessage}>
      {children}
    </WebSocketContext.Provider>
  );
}

// Hook helper (boa prática)
export function useRealtimeData() {
  return useContext(WebSocketContext);
}