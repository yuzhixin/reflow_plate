"""Generate 16x16 Chinese font bitmap data using Windows GDI via ctypes."""
import ctypes
from ctypes import wintypes

# Windows GDI constants
ANSI_CHARSET = 0
DEFAULT_CHARSET = 1
FW_NORMAL = 400
OUT_TT_PRECIS = 4
CLIP_DEFAULT_PRECIS = 0
DEFAULT_QUALITY = 0
DEFAULT_PITCH = 0
ANTIALIASED_QUALITY = 4
CLEARTYPE_QUALITY = 5

# Load GDI32
gdi32 = ctypes.windll.gdi32
user32 = ctypes.windll.user32

# Get DC
hdc = user32.GetDC(0)
memdc = gdi32.CreateCompatibleDC(hdc)

# Create font
hfont = gdi32.CreateFontW(
    16, 0, 0, 0, FW_NORMAL, 0, 0, 0,
    DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
    ANTIALIASED_QUALITY, DEFAULT_PITCH, "SimHei"
)

# Select font
old_font = gdi32.SelectObject(memdc, hfont)

# Create bitmap
bmp = gdi32.CreateCompatibleBitmap(hdc, 16, 16)
old_bmp = gdi32.SelectObject(memdc, bmp)

# Characters we need
chars = "待机恒温模式回流焊设置错误返锡类型低中高目标度参数保存退出开始正常异常预热冷却完成传感器开路全帧跳变已无更改长按"

# Bitmap info structure for 32-bit
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

print("// Auto-generated 16x16 Chinese font bitmaps (SimHei)")
print("// Each character: 32 bytes (16 rows, 2 bytes per row, MSB=left)")
print("")
print("typedef struct {")
print("    const char *utf8;")
print("    const uint8_t bitmap[32];")
print("} ChineseChar;")
print("")
print(f"#define CHINESE_FONT_COUNT {len(chars)}")
print("")
print("static const ChineseChar chinese_font[CHINESE_FONT_COUNT] = {")

for ch in chars:
    # Clear background
    brush = gdi32.CreateSolidBrush(0x00000000)  # Black
    gdi32.SelectObject(memdc, brush)
    rect = wintypes.RECT(0, 0, 16, 16)
    user32.FillRect(memdc, ctypes.byref(rect), brush)
    gdi32.DeleteObject(brush)

    # Set text color to white
    gdi32.SetTextColor(memdc, 0x00FFFFFF)
    gdi32.SetBkColor(memdc, 0x00000000)

    # Draw the character
    gdi32.TextOutW(memdc, 0, 0, ch, 1)

    # Read pixels
    bi = BITMAPINFOHEADER()
    bi.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bi.biWidth = 16
    bi.biHeight = -16  # Top-down
    bi.biPlanes = 1
    bi.biBitCount = 32
    bi.biCompression = 0  # BI_RGB

    pixels = (ctypes.c_uint32 * (16 * 16))()
    gdi32.GetDIBits(memdc, bmp, 0, 16, ctypes.byref(pixels), ctypes.byref(bi), 0)

    # Convert to 1-bit bitmap (16 rows x 2 bytes)
    bitmap = []
    for row in range(16):
        byte1 = 0
        byte2 = 0
        for col in range(16):
            px = pixels[row * 16 + col]
            # Check if pixel is bright (non-black)
            if px & 0x00FFFFFF:  # Ignore alpha
                if col < 8:
                    byte1 |= (1 << (7 - col))
                else:
                    byte2 |= (1 << (15 - col))
        bitmap.append(byte1)
        bitmap.append(byte2)

    # Print as C array
    hex_str = ", ".join(f"0x{b:02X}" for b in bitmap)
    print(f"    {{\"{ch}\", {{{hex_str}}}}},")

print("};")

# Cleanup
gdi32.SelectObject(memdc, old_font)
gdi32.SelectObject(memdc, old_bmp)
gdi32.DeleteObject(hfont)
gdi32.DeleteObject(bmp)
gdi32.DeleteDC(memdc)
user32.ReleaseDC(0, hdc)