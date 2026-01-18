import { createContext, useContext, useState } from "react";

const DashboardContext = createContext(null);

export function DashboardProvider({ children }) {
  const [mode, setMode] = useState("stream"); // stream | filter
  const [filterData, setFilterData] = useState(null);

  return (
    <DashboardContext.Provider
      value={{ mode, setMode, filterData, setFilterData }}
    >
      {children}
    </DashboardContext.Provider>
  );
}

export function useDashboard() {
  return useContext(DashboardContext);
}