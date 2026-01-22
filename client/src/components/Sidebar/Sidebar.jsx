import { useState } from 'react';
import './Sidebar.css';

export default function Navbar() {
    // Criar um estado para controlar se está aberto ou não
    const [isOpen, setIsOpen] = useState(false);

    // Função para alternar o estado
    const handleToggle = () => {
        setIsOpen(!isOpen);
    };

    return (
        // Aplicar a classe 'open-sidebar' condicionalmente
        <nav id="sidebar" className={isOpen ? 'open-sidebar' : ''}>
            <div id="sidebar_content">
                <div id="user">
                    <img src="/logo192.png" id="user_avatar" alt="Avatar" />
        
                    <p id="user_infos">
                        <span className="item-description">
                            Solar Station
                        </span>
                        <span className="item-description">
                            App Web
                        </span>
                    </p>
                </div>
        
                <ul id="side_items">
                    <li className="side-item active">
                        <a href="#">
                            <i className="fa-solid fa-chart-line"></i>
                            <span className="item-description">Dashboard</span>
                        </a>
                    </li>
                    <li className="side-item">
                        <a href="#">
                            <i className="fa-solid fa-user"></i>
                            <span className="item-description">
                                Usuários
                            </span>
                        </a>
                    </li>
        
                    <li className="side-item">
                        <a href="#">
                            <i className="fa-solid fa-bell"></i>
                            <span className="item-description">
                                Notificações
                            </span>
                        </a>
                    </li>
        
                    <li className="side-item">
                        <a href="#">
                            <i className="fa-solid fa-box"></i>
                            <span className="item-description">
                                Produtos
                            </span>
                        </a>
                    </li>
        
                    <li className="side-item">
                        <a href="#">
                            <i className="fa-solid fa-image"></i>
                            <span className="item-description">
                                Imagens
                            </span>
                        </a>
                    </li>
        
                    <li className="side-item">
                        <a href="#">
                            <i className="fa-solid fa-gear"></i>
                            <span className="item-description">
                                Configurações
                            </span>
                        </a>
                    </li>
                </ul>
        
                {/* Adicionar o evento onClick no botão */}
                <button id="open_btn" onClick={handleToggle}>
                    <i 
                        id="open_btn_icon" 
                        className="fa-solid fa-chevron-right"
                    ></i>
                </button>
            </div>

            <div id="logout">
                <button id="logout_btn">
                    <i className="fa-solid fa-right-from-bracket"></i>
                    <span className="item-description">Logout</span>
                </button>
            </div>
        </nav>
    );
}