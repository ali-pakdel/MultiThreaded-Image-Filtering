#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <chrono>
#include <pthread.h>

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::vector;
using namespace std::chrono;

#pragma pack(1)
#pragma once

#define OUTPUT_FILE "output.bmp"
#define NO_THREADS 4
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

struct Ranges
{
  int row_start;
  int row_last;
  int col_start;
  int col_last;
  int end;
};
Ranges ranges[NO_THREADS];

int rows;
int cols;

char *fileBuffer;
int bufferSize;

unsigned char** red;
unsigned char** green;
unsigned char** blue;

vector<vector<unsigned char>> red_copy;
vector<vector<unsigned char>> green_copy;
vector<vector<unsigned char>> blue_copy;

float reds_mean = 0;
float greens_mean = 0;
float blues_mean = 0;

bool fillAndAllocate(const char *fileName, int &rows, int &cols);

void getPixlesFromBMP24(Ranges arguments);
void* getPixlesBridge(void* _id);
void getPixlesMultiThread();

void writeOutBmp24(Ranges arguments);
void* writeBridge(void* _id);
void writeMultiThread();

void mAllocColors();

void calculateChannelsMean();

void calculateRanges();

bool isInRange(int x, int limit);

void smoothing();
void* smoothingBridge(void* _id);
void smoothingMultiThread();

void sepia(Ranges arguments);
void* sepiaBridge(void* _id);
void sepiaMultiThread();

void washedOut(Ranges arguments);
void* washedOutBridge(void* _id);
void washedOutMultiThread();

void crossSign();

void makeCopyRGB();

void applyFilters();


bool fillAndAllocate(const char *fileName, int &rows, int &cols)
{
  std::ifstream file(fileName);

  if (file)
  {
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    fileBuffer = new char[length];
    file.read(&fileBuffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&fileBuffer[0]);
    info_header = (PBITMAPINFOHEADER)(&fileBuffer[0] + sizeof(BITMAPFILEHEADER));
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

void getPixlesFromBMP24(Ranges arguments)
{
  int count = 1 + (arguments.row_start * cols * 3);
  int extra = cols % 4;
  for (int i = arguments.row_start; i < arguments.row_last; i++)
  {
    count += extra + (cols - arguments.col_last) * 3;
    for (int j = arguments.col_last - 1; j >= arguments.col_start; j--)
    {
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0: 
          red[i][j] = fileBuffer[bufferSize - count];
          count++;     
          break;
        case 1:
          green[i][j] = fileBuffer[arguments.end - count];
          count++;
          break;
        case 2:
          blue[i][j] = fileBuffer[arguments.end - count];
          count++;
          break;
        }
      }
    }
    count += arguments.col_start * 3;
  }
}
void* getPixlesBridge(void* _id)
{
  int id = *((int*) _id);
  Ranges arguments = ranges[id];
  getPixlesFromBMP24(arguments);
  pthread_exit(NULL);
}
void getPixlesMultiThread()
{
  pthread_t threads[NO_THREADS];
  int id_threads[NO_THREADS];

  for (int i = 0; i < NO_THREADS; i++)
  {
    id_threads[i] = i;
    int thc = pthread_create(&threads[i], NULL, getPixlesBridge, (void*)&id_threads[i]);
    if (thc != 0)
    {
      cout << "getPixlesMultiThread: Error in creating thread!" << endl;
      exit(0);
    }
  }

  for (int i = 0; i < NO_THREADS; i++)
  {
    int thj = pthread_join(threads[i], NULL);
    if (thj != 0)
    {
      cout << "getPixlesMultiThread: Error in joining thread!" << endl;
      exit(0);
    }
  }
}

void writeOutBmp24(Ranges arguments)
{
  int count = 1 + (arguments.row_start * cols * 3);
  int extra = cols % 4;
  for (int i = arguments.row_start; i < arguments.row_last; i++)
  {
    count += extra + (cols - arguments.col_last) * 3;
    for (int j = arguments.col_last - 1; j >= arguments.col_start; j--)
    {
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          // cout << red[i][j] << endl;
          fileBuffer[bufferSize - count] = red[i][j];
          count++;
          break;
        case 1:
          // cout << green[i][j] << endl;
          fileBuffer[bufferSize - count] = green[i][j];
          count++;
          break;
        case 2:
          // cout << blue[i][j] << endl;
          fileBuffer[bufferSize - count] = blue[i][j];
          count++;
          break;
        }
      }
    }
    count+= arguments.col_start * 3;
  }
}
void* writeBridge(void* _id)
{
  int id = *((int*) _id);
  Ranges arguments = ranges[id];
  writeOutBmp24(arguments);
  pthread_exit(NULL);
}
void writeMultiThread()
{
  pthread_t threads[NO_THREADS];
  int id_threads[NO_THREADS];

  for (int i = 0; i < NO_THREADS; i++)
  {
    id_threads[i] = i;
    int thc = pthread_create(&threads[i], NULL, writeBridge, (void*)&id_threads[i]);
    if (thc != 0)
    {
      cout << "writeMultiThread: Error in creating thread!" << endl;
      exit(0);
    }
  }

  for (int i = 0; i < NO_THREADS; i++)
  {
    int thj = pthread_join(threads[i], NULL);
    if (thj != 0)
    {
      cout << "writeMultiThread: Error in joining thread!" << endl;
      exit(0);
    }
  }
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

void calculateChannelsMean()
{
  float red_sum_ = 0;
  float green_sum_ = 0;
  float blue_sum_ = 0;
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      red_sum_ += red[i][j];
      green_sum_ += green[i][j];
      blue_sum_ += blue[i][j];
    }
  }
  int count = rows * cols;
  reds_mean = red_sum_ / count;
  greens_mean = green_sum_ / count;
  blues_mean = blue_sum_ / count;
}

void calculateRanges()
{
  for (int i = 0; i < NO_THREADS; i++)
  {
    ranges[i].end = bufferSize;
    if (i == 0 || i == 1)
    {
      ranges[i].row_start = 0;
      ranges[i].col_start = i * (cols / 2);
    }
    if (i == 2 || i == 3)
    {
      ranges[i].row_start = rows / 2;
      ranges[i].col_start = (i - 2) * (cols / 2);
    }
    ranges[i].row_last = ranges[i].row_start + (rows / 2);
    ranges[i].col_last = ranges[i].col_start + (cols / 2);
  }
}

bool isInRange(int x, int limit)
{
  if (x > 0 && x < limit)
    return true;
  else
    return false;
}

void smoothing(Ranges arguments)
{
  int row_moves[8] = {0, -1, -1, -1, 0, 1, 1, 1};
  int col_moves[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
  int reds_sum, greens_sum, blues_sum;
  
  for (int i = arguments.row_start; i < arguments.row_last; i++)
  {
    for (int j = arguments.col_start; j < arguments.col_last; j++)
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
void* smoothingBridge(void* _id)
{
  int id = *((int *) _id);
  Ranges arguments = ranges[id];
  smoothing(arguments);
  pthread_exit(NULL);
}
void smoothingMultiThread()
{
  pthread_t threads[NO_THREADS];
  int id_threads[NO_THREADS];

  for (int i = 0; i < NO_THREADS; i++)
  {
    id_threads[i] = i;
    int thc = pthread_create(&threads[i], NULL, smoothingBridge, (void*)&id_threads[i]);
    if (thc != 0)
    {
      cout << "smoothMultiThread: Error in creating thread!" << endl;
      exit(0);
    }
  }

  for (int i = 0; i < NO_THREADS; i++)
  {
    int thj = pthread_join(threads[i], NULL);
    if (thj != 0)
    {
      cout << "smoothMultiThread: Error in joining thread!" << endl;
      exit(0);
    }
  }
}

void sepia(Ranges arguments)
{
  float new_red, new_green, new_blue;
  for (int i = arguments.row_start; i < arguments.row_last; i++)
  {
    for (int j = arguments.col_start; j < arguments.col_last; j++)
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
void* sepiaBridge(void* _id)
{
  int id = *((int *) _id);
  Ranges arguments = ranges[id];
  sepia(arguments);
  pthread_exit(NULL);
}
void sepiaMultiThread()
{
  pthread_t threads[NO_THREADS];
  int id_threads[NO_THREADS];

  for (int i = 0; i < NO_THREADS; i++)
  {
    id_threads[i] = i;
    int thc = pthread_create(&threads[i], NULL, sepiaBridge, (void*)&id_threads[i]);
    if (thc != 0)
    {
      cout << "sepiaMultiThread: Error in creating thread!" << endl;
      exit(0);
    }
  }

  for (int i = 0; i < NO_THREADS; i++)
  {
    int thj = pthread_join(threads[i], NULL);
    if (thj != 0)
    {
      cout << "sepiaMultiThread: Error in joining thread!" << endl;
      exit(0);
    }
  }
}

void washedOut(Ranges arguments)
{
  for (int i = arguments.row_start; i < arguments.row_last; i++)
  {
    for (int j = arguments.col_start; j < arguments.col_last; j++)
    {
      red[i][j] = (red[i][j] * 0.4) + (reds_mean * 0.6);
      green[i][j] = (green[i][j] * 0.4) + (greens_mean * 0.6);      
      blue[i][j] = (blue[i][j] * 0.4) + (blues_mean * 0.6);
    }
  }
}
void* washedOutBridge(void* _id)
{
  int id = *((int *) _id);
  Ranges arguments = ranges[id];
  washedOut(arguments);
  pthread_exit(NULL);
}
void washedOutMultiThread()
{
  pthread_t threads[NO_THREADS];
  int id_threads[NO_THREADS];

  for (int i = 0; i < NO_THREADS; i++)
  {
    id_threads[i] = i;
    int thc = pthread_create(&threads[i], NULL, washedOutBridge, (void*)&id_threads[i]);
    if (thc != 0)
    {
      cout << "washedOutMultiThread: Error in creating thread!" << endl;
      exit(0);
    }
  }

  for (int i = 0; i < NO_THREADS; i++)
  {
    int thj = pthread_join(threads[i], NULL);
    if (thj != 0)
    {
      cout << "washedOutMultiThread: Error in joining thread!" << endl;
      exit(0);
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
  smoothingMultiThread();
  sepiaMultiThread();
  calculateChannelsMean();
  washedOutMultiThread();
  crossSign();
}

int main(int argc, char *argv[])
{
  auto start = high_resolution_clock::now();
  
  char *fileName = argv[1];
  if (!fillAndAllocate(fileName, rows, cols))
  {
    cout << "File read error" << endl;
    return 0;
  }

  mAllocColors();
  calculateRanges();
  getPixlesMultiThread();
  applyFilters();

  std::ofstream write(OUTPUT_FILE);
  if (!write)
  {
    cout << "Failed to write out.bmp" << endl;
    return 0;
  }
  writeMultiThread();
  write.write(fileBuffer, bufferSize);
  
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);
  cout << "Time taken for parallel execution: "
         << duration.count() << " miliseconds" << endl;
  
  return 0;
}