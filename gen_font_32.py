"""Generate 32x32 Chinese font bitmap data using Windows GDI via ctypes."""
import ctypes
from ctypes import wintypes

SIZE = 32

# Windows GDI constants
ANSI_CHARSET = 0
DEFAULT_CHARSET = 1
FW_NORMAL = 400
OUT_TT_PRECIS = 4
CLIP_DEFAULT_PRECIS = 0
DEFAULT_PITCH = 0
ANTIALIASED_QUALITY = 4

gdi32 = ctypes.windll.gdi32
user32 = ctypes.windll.user32

hdc = user32.GetDC(0)
memdc = gdi32.CreateCompatibleDC(hdc)

hfont = gdi32.CreateFontW(
    SIZE, 0, 0, 0, FW_NORMAL, 0, 0, 0,
    DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
    ANTIALIASED_QUALITY, DEFAULT_PITCH, "SimHei"
)

old_font = gdi32.SelectObject(memdc, hfont)

bmp = gdi32.CreateCompatibleBitmap(hdc, SIZE, SIZE)
old_bmp = gdi32.SelectObject(memdc, bmp)

# Characters we need for main menu
chars = "恒温回流焊设置"

class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wintypes.DWORD),
        ("biWidth", wintypes.LONG),
        ("biHeight", wintypes.LONG),
        ("biPlanes", wintypes.WORD),
        ("biBitCount", wintypes.WORD),
        ("biCompression", wintypes.DWORD),
        ("biSizeImage", wintypes.DWORD),
        ("biXPelsPerMeter", wintypes.LONG),
        ("biYPelsPerMeter", wintypes.LONG),
        ("biClrUsed", wintypes.DWORD),
        ("biClrImportant", wintypes.DWORD),
    ]

print("// Auto-generated 32x32 Chinese font bitmaps (SimHei)")
print("// Each character: 128 bytes (32 rows, 4 bytes per row, MSB=left)")
print("")

for ch in chars:
    brush = gdi32.CreateSolidBrush(0x00000000)
    gdi32.SelectObject(memdc, brush)
    rect = wintypes.RECT(0, 0, SIZE, SIZE)
    user32.FillRect(memdc, ctypes.byref(rect), brush)
    gdi32.DeleteObject(brush)

    gdi32.SetTextColor(memdc, 0x00FFFFFF)
    gdi32.SetBkColor(memdc, 0x00000000)

    gdi32.TextOutW(memdc, 0, 0, ch, 1)

    bi = BITMAPINFOHEADER()
    bi.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bi.biWidth = SIZE
    bi.biHeight = -SIZE
    bi.biPlanes = 1
    bi.biBitCount = 32
    bi.biCompression = 0

    pixels = (ctypes.c_uint32 * (SIZE * SIZE))()
    gdi32.GetDIBits(memdc, bmp, 0, SIZE, ctypes.byref(pixels), ctypes.byref(bi), 0)

    # Convert to 1-bit bitmap: 32 rows x 4 bytes
    bitmap = []
    for row in range(SIZE):
        bytes_row = [0, 0, 0, 0]
        for col in range(SIZE):
            px = pixels[row * SIZE + col]
            if px & 0x00FFFFFF:
                byte_idx = col // 8
                bit_idx = 7 - (col % 8)
                bytes_row[byte_idx] |= (1 << bit_idx)
        bitmap.extend(bytes_row)

    hex_str = ", ".join(f"0x{b:02X}" for b in bitmap)
    print(f"// {ch}")
    print(f"{{{hex_str}}},")
    print("")

# Cleanup
gdi32.SelectObject(memdc, old_font)
gdi32.SelectObject(memdc, old_bmp)
gdi32.DeleteObject(hfont)
gdi32.DeleteObject(bmp)
gdi32.DeleteDC(memdc)
user32.ReleaseDC(0, hdc)