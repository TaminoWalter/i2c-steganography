# I2C Steganography Tool

Steganography tool for the Introduction to C Programming module at HSLU

# Team Members

* Anujan Sivakurunathan
* Kevin Krummenacher [Github](https://github.com/kevkru)
* Tamino Walter [Github](https://github.com/TaminoWalter)

# Deliverables
* stego.exe (for Windows x86)
* Optional:
  + stego (for Linux/Mac)
  + sample.png
  + sample.bmp
  + topSecret.txt

# How to use
```
This steganography tool allows you to hide text on images or to read hidden text from images.

Usage:
stego [command]

Available Commands:
embed     Hides some content inside an image
extract   Extracts some hidden content from an image
capacity  Get capacity of a file

Options:
-h, --help  Show this help dialog

Use "stego [command] --help" for more information about a command.
```

# Run on Windows
```
stego.exe --help
stego.exe embed sample.bmp ThisIsTopSecret
stego.exe extract out.bmp
```

# Dependencies & Acknowledgments
While the BMP processing was implemented from scratch to demonstrate low-level file manipulation, we utilize external libraries for complex image compression formats (in this case for PNG):

* **stb_image** (v2.30) & **stb_image_write** (v1.16) -  We use the excellent single-file public domain libraries by [Sean Barrett](https://github.com/nothings) to handle PNG reading and writing without external dependencies like libpng.