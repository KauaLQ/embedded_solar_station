import React from "react";
import TimeFilter from "../TimeFilter/TimeFilter";
import './Navbar.css';

export default function Navbar(){
    return (
        <nav className="myNav">
            <div>
                <TimeFilter />
            </div>
        </nav>
    );
}