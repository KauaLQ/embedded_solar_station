import TimeFilter from "../TimeFilter/TimeFilter";
import './Navbar.css';

export default function Navbar() {
    return (
        <nav id="container-mynavbar">
            <div id="time_filter">
                <TimeFilter />
            </div>
        </nav>
    );
}