# MultiThreaded Image Filtering
This project focuses on image filtering using C++ with multi-threading capabilities. The implemented filters encompass smoothing, sepia tone, washed-out effects, and the addition of a cross pattern. The application processes BMP image files efficiently, utilizing both single and multiple threads for enhanced performance, ultimately saving the modified image as a BMP file.
## Project Structure
- **readImg.cpp**: This file houses the primary source code, encapsulating the image filtering algorithms and the multi-threading implementation.
- **Makefile**: The Makefile simplifies the compilation process for the project.

## How to Use
1. Ensure you possess a BMP image file (e.g., image.bmp) in the same directory as the executable.
2. Execute the compiled program, named ImageFilters.out, with the input BMP image file specified as a command-line argument:
   ```bash
   ./ImageFilters.out image.bmp
   ```
3. The application will then apply a range of filters to the input image, creating an output BMP image named output.bmp.
