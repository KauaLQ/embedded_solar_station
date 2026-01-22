import TimeFilter from "../TimeFilter/TimeFilter";
import './Navbar.css';

export default function Navbar() {
    return (
        <nav id="container-mynavbar">
            <div id="time_filter">
                <TimeFilter />
            </div>

            <div id="user_acount">
                <div id="crop_container">
                    <img src="/user_photo.jpg" className="user-acount-avatar" alt="Avatar" />
                </div>

                <div id="user_acount_infos">
                    <span className="item-acount-description">
                        Administrador
                    </span>
                    <span className="item-acount-description">
                        Conta Dev
                    </span>
                </div>
            </div>
        </nav>
    );
}