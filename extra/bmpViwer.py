import numpy as np
import matplotlib.pyplot as plt

# Seus dados de entrada
data = [
    0xDF, 0x8B, 0x91, 0xE3, 0xC7, 0x8B, 0x13, 0x23, 0x63, 0xE3, 0x63, 0xC7, 0xC7, 0x8F, 0x8F, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF8, 0xDD, 0x8E, 0x8C, 0xD8, 0xF1, 0xE2, 0xE7, 0xFF, 0xFF, 0xFF
]

# Configurações da imagem
width = 16
height = 16

def draw_bitmap(data, w, h):
    # Criar uma matriz vazia (h, w)
    img = np.zeros((h, w))
    
    # Preencher a matriz processando byte a byte
    # Assumindo orientação vertical (SSD1306/U8g2 standard)
    idx = 0
    for page in range(0, h // 8):
        for x in range(0, w):
            if idx < len(data):
                byte = data[idx]
                # Extrair os 8 bits do byte para os pixels verticais
                for bit in range(8):
                    if (byte >> bit) & 1:
                        img[page * 8 + bit, x] = 1
                idx += 1

    # Plotar o resultado
    plt.figure(figsize=(10, 5))
    plt.imshow(img, cmap='binary', interpolation='nearest')
    plt.title(f"Bitmap Preview ({w}x{h})")
    plt.axis('off')
    plt.show()

if __name__ == "__main__":
    draw_bitmap(data, width, height)