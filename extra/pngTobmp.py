from PIL import Image

# ================= CONFIGURAÇÕES =================
WIDTH  = 16   # largura do bitmap (múltiplo de 8 recomendado)
HEIGHT = 16    # altura do bitmap (OBRIGATORIAMENTE múltiplo de 8)
invert = True  # True = imagem branca no OLED, False = imagem preta no OLED
# =================================================

# Validação para não quebrar a lógica de páginas
if HEIGHT % 8 != 0:
    raise ValueError("HEIGHT deve ser múltiplo de 8 para displays OLED paginados.")

PAGES = HEIGHT // 8

img = (Image.open(r"C:\EMBARCATECH\solar_station_v2\extra\nocloud.png").convert("1").resize((WIDTH, HEIGHT)))

data = list(img.getdata())

buf = []
for page in range(PAGES):
    for x in range(WIDTH):
        byte = 0
        for bit in range(8):
            y = page * 8 + bit
            pixel = data[y * WIDTH + x]

            if invert:
                # Branco vira pixel aceso no OLED
                if pixel != 0:
                    byte |= (1 << bit)
            else:
                # Preto vira pixel aceso no OLED
                if pixel == 0:
                    byte |= (1 << bit)

        buf.append(byte)

print(", ".join(f"0x{b:02X}" for b in buf))