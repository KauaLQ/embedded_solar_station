import numpy as np
import matplotlib.pyplot as plt

# Seus dados de entrada
data = [
    0xFF, 0x7F, 0x3B, 0x37, 0x6F, 0x5F, 0xA7, 0x47, 0x87, 0x07, 0x0F, 0x0F, 0x3F, 0x7F, 0xFF, 0xFF, 0xFD, 0xF0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE1, 0xE2, 0xE5, 0xEA, 0xF4, 0xE8, 0xD0, 0xF0, 0xFF
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