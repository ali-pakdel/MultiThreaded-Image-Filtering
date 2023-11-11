#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <chrono>

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::vector;
using namespace std::chrono;

#pragma pack(1)
#pragma once

#define OUTPUT_FILE "output.bmp"
#define COLOR_LIMIT 255

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef struct tagBITMAPFILEHEADER
{
  WORD bfType;
  DWORD bfSize;
  WORD bfReserved1;
  WORD bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

int rows;
int cols;

unsigned char** red;
unsigned char** green;
unsigned char** blue;

vector<vector<unsigned char>> red_copy;
vector<vector<unsigned char>> green_copy;
vector<vector<unsigned char>> blue_copy;

float reds_mean = 0;
float greens_mean = 0;
float blues_mean = 0;

void mAllocColors();
bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize);
void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer);
void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize);

bool isInRange(int x, int limit);
void makeCopyRGB();
void calculateChannelMeans();

void applyFilters();
void smoothing();
void sepia();
void washedOut();
void crossSign();

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize)
{
  std::ifstream file(fileName);

  if (file)
  {
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&buffer[0]);
    info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
    rows = info_header->biHeight;
    cols = info_header->biWidth;
    bufferSize = file_header->bfSize;
    return 1;
  }
  else
  {
    cout << "File" << fileName << " doesn't exist!" << endl;
    return 0;
  }
}

void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer)
{
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0: 
          red[i][j] = fileReadBuffer[end - count];        
          count++;
          break;
        case 1:
          green[i][j] = fileReadBuffer[end - count];
          count++;
          break;
        case 2:
          blue[i][j] = fileReadBuffer[end - count];
          count++;
          break;
        }
      }
  }
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize)
{
  std::ofstream write(nameOfFileToCreate);
  if (!write)
  {
    cout << "Failed to write " << nameOfFileToCreate << endl;
    return;
  }
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          fileBuffer[bufferSize - count] = red[i][j];
          count++;
          break;
        case 1:
          fileBuffer[bufferSize - count] = green[i][j];
          count++;
          break;
        case 2:
          fileBuffer[bufferSize - count] = blue[i][j];
          count++;
          break;
        }
      }
  }
  write.write(fileBuffer, bufferSize);
}

void mAllocColors() 
{
  red = new unsigned char*[rows];
  green = new unsigned char*[rows];
  blue = new unsigned char*[rows];
    for (int i = 0; i < rows; i++)
    {
      red[i] = new unsigned char[cols];
      green[i] = new unsigned char[cols];
      blue[i] = new unsigned char[cols];
    }
}

bool isInRange(int x, int limit)
{
  if (x > 0 && x < limit)
    return true;
  else
    return false;
}

void calculateChannelMeans()
{
  float reds_sum = 0;
  float greens_sum = 0;
  float blues_sum = 0;
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      reds_sum += red[i][j];
      greens_sum += green[i][j];
      blues_sum += blue[i][j];
    }
  }
  int count = rows * cols;
  reds_mean = reds_sum / count;
  greens_mean = greens_sum / count;
  blues_mean = blues_sum / count;
}

void makeCopyRGB()
{
  vector<vector<unsigned char>> red_copy_(rows, vector<unsigned char>(cols));
  vector<vector<unsigned char>> green_copy_(rows, vector<unsigned char>(cols));
  vector<vector<unsigned char>> blue_copy_(rows, vector<unsigned char>(cols));

  for (int i = 0; i < rows; i++)  
  {
    for (int j = 0; j < cols; j++)
    {
      red_copy_[i][j] = (*(red[i] + j));
      green_copy_[i][j] = (*(green[i] + j));
      blue_copy_[i][j] = (*(blue[i] + j));
    }
  }
  red_copy = red_copy_;
  green_copy = green_copy_;
  blue_copy = blue_copy_;
}

void applyFilters()
{
  makeCopyRGB();
  smoothing();
  sepia();
  calculateChannelMeans();
  washedOut();
  crossSign();
}

void smoothing()
{
  int row_moves[8] = {0, -1, -1, -1, 0, 1, 1, 1};
  int col_moves[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

  int reds_sum, greens_sum, blues_sum;
  
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      reds_sum = greens_sum = blues_sum = 0;
      for (int k = 0; k < 8; k++)
      {
        if (isInRange(i + row_moves[k], rows) && isInRange(j + col_moves[k], cols))
        {
          reds_sum += int(red_copy[i + row_moves[k]][j + col_moves[k]]);
          greens_sum += int(green_copy[i + row_moves[k]][j + col_moves[k]]);
          blues_sum += int(blue_copy[i + row_moves[k]][j + col_moves[k]]);
        }
      }
      red[i][j] = char((reds_sum + red_copy[i][j]) / 9);
      green[i][j] = char((greens_sum + green_copy[i][j]) / 9);
      blue[i][j] = char((blues_sum + blue_copy[i][j]) / 9);
    }
  }
}

void sepia()
{
  float new_red, new_green, new_blue;
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      new_red = (red[i][j] * 0.393) + (green[i][j] * 0.769) + (blue[i][j] * 0.189);
      if (new_red > COLOR_LIMIT)
        new_red = COLOR_LIMIT;

      new_green = (red[i][j] * 0.349) + (green[i][j] * 0.686) + (blue[i][j] * 0.168);
      if (new_green > COLOR_LIMIT)
        new_green = COLOR_LIMIT;

      new_blue = (red[i][j] * 0.272) + (green[i][j] * 0.534) + (blue[i][j] * 0.131);
      if (new_blue > COLOR_LIMIT)
        new_blue = COLOR_LIMIT;

      red[i][j] = new_red;
      green[i][j] = new_green;
      blue[i][j] = new_blue;
    }
  }
}

void washedOut()
{
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      red[i][j] = (red[i][j] * 0.4) + (reds_mean * 0.6);
      green[i][j] = (green[i][j] * 0.4) + (greens_mean * 0.6);      
      blue[i][j] = (blue[i][j] * 0.4) + (blues_mean * 0.6);
    }
  }
}

void crossSign()
{
  for (int i = 0; i < rows; i++)
  {
    red[i][i] = green[i][i] = blue[i][i] = COLOR_LIMIT;
    if (i != rows - 1)
      red[i][i + 1] = green[i][i + 1] = blue[i][i + 1] = COLOR_LIMIT;
    if (i != 0)
      red[i][i - 1] = green[i][i - 1] = blue[i][i - 1] = COLOR_LIMIT;
    
    red[i][cols - i] = green[i][cols - i] = blue[i][cols - i] = COLOR_LIMIT;
    if (cols - i != COLOR_LIMIT)
      red[i][cols - i + 1] = green[i][cols - i + 1] = blue[i][cols - i + 1] = COLOR_LIMIT;
    if (cols - i != 0)
      red[i][cols - i - 1] = green[i][cols - i - 1] = blue[i][cols - i - 1] = COLOR_LIMIT;
  }
}

int main(int argc, char *argv[])
{
  auto start = high_resolution_clock::now();
  
  char *fileBuffer;
  int bufferSize;
  char *fileName = argv[1];
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize))
  {
    cout << "File read error" << endl;
    return 1;
  }

  mAllocColors();
  getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer);
  applyFilters();
  writeOutBmp24(fileBuffer, OUTPUT_FILE, bufferSize);
  
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);
  cout << "Time taken for serial execution: "
         << duration.count() << " miliseconds" << endl;
  
  return 0;
}
