#include <iostream>
#include <fstream>
#include <vector>


// Определение структуры BMPHeader, описывающей заголовок BMP-файла
#pragma pack(push, 1)
struct BMPHeader
{
    uint16_t signature;
    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t dataOffset;
    uint32_t headerSize;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    uint32_t compression;
    uint32_t imageSize;
    int32_t  xPixelsPerMeter;
    int32_t  yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};
#pragma pack(pop)

// Класс ImageProcessor для обработки и сохранения изображения
class ImageProcessor
{
public:
    ImageProcessor(const std::string& filename);
    ~ImageProcessor();

    bool LoadImage();
    bool Rotate1();
    bool Rotate2();
    bool ApplyGaussianFilter();
    bool SaveImage(const std::string& filename);

private:
    BMPHeader header;
    std::vector<uint8_t> buffer; // Буфер с данными изображения
    std::vector<uint8_t> ImageData; 
    int width;
    int height;
    int rowSize;
    int newRowSize;
    int padding;

    // Применение гауссова фильтра к изображению
    void ApplyGaussianFilter(std::vector<uint8_t>& outputBuffer, const std::vector<uint8_t>& inputBuffer, int width, int height);
    // Применение гауссового ядра к пикселю изображения
    double ApplyGaussianKernel(const std::vector<uint8_t>& inputBuffer, const double kernel[3][3], int width, int height, int x, int y);
};


ImageProcessor::ImageProcessor(const std::string& filename)
{
    // Конструктор класса ImageProcessor
}

ImageProcessor::~ImageProcessor()
{
    // Деструктор класса ImageProcessor
}

// Метод, загружающий изображение из файла в память
bool ImageProcessor::LoadImage()
{
    std::ifstream is("Picture.bmp", std::ifstream::binary);

    if (!is.is_open())
    {
        std::cerr << "Error opening the file." << std::endl;
        return 1;
    }

    is.read(reinterpret_cast<char*>(&header), sizeof(BMPHeader));

    if (header.signature != 0x4D42)
    {
        std::cerr << "The file is not a BMP image." << std::endl;
        is.close();
        return 1;
    }

    if (header.bitsPerPixel != 8)
    {
        std::cerr << "The image should have a color depth of 8 bits." << std::endl;
        is.close();
        return 1;
    }

    width = header.width;
    height = header.height;

    padding = (4 - (width % 4)) & 3; // Рассчет количества паддинга
    rowSize = width * (header.bitsPerPixel / 8) + padding; // Рассчет полного размера строки
    int bufferSize = rowSize * height; // Полный размер буфера

    buffer.resize(bufferSize);

    int dataBetween = header.dataOffset - sizeof(BMPHeader); // Размер данных между заголовком и пиксельными данными
    ImageData.resize(dataBetween);
    is.read(reinterpret_cast<char*>(ImageData.data()), dataBetween);

    is.seekg(header.dataOffset); // Перемещение указателя в файле к данным изображения

    // Чтение данных изображения из файла
    for (int i = 0; i < height; i++)
    {
        is.read(reinterpret_cast<char*>(&buffer[i * rowSize]), rowSize);
    }

    is.close();

    std::cout << "Allocated memory size for image loading: " << header.fileSize << " bytes" << std::endl;


    for (int y = 0; y < header.height; y++) 
    {
        for (int x = 0; x < header.width; x++)
        {
            uint8_t pixelValue = buffer[y * rowSize + x];
            std::cout << static_cast<int>(pixelValue) << " ";
        }
        std::cout << std::endl;
    }

    return true;
}

// Метод, выполняющий поворот изображения на 90 градусов по часовой стрелке
bool ImageProcessor::Rotate1()
{
    int newWidth = height;
    int newHeight = width;
    std::vector<uint8_t> rotatedBuffer(buffer.size());
    padding = (4 - (newWidth % 4)) & 3;
    newRowSize = newWidth + padding; // Новый размер строки с учетом паддинга


    for (int y = 0; y < newHeight; ++y)
    {
        for (int x = 0; x < newWidth; ++x)
        {
            int oldX = y;
            int oldY = newWidth - x - 1;
            int newIndex = y * newRowSize + x;  
            int oldIndex = oldY * rowSize + oldX;
            rotatedBuffer[newIndex] = buffer[oldIndex];
        }
    }

    buffer = rotatedBuffer;
    width = newWidth;
    height = newHeight;

    // Обновление размера строки и паддинга в заголовке BMP
    header.width = width;
    header.height = height;
    header.fileSize = sizeof(BMPHeader) + buffer.size();

    return true;
}


// Метод, выполняющтй поворот изображения на 90 градусов против часовой стрелки
bool ImageProcessor::Rotate2()
{
    int newWidth = height;
    int newHeight = width;
    std::vector<uint8_t> rotatedBuffer(buffer.size());
    padding = (4 - (newWidth % 4)) & 3;
    newRowSize = newWidth + padding;

    for (int y = 0; y < newHeight; ++y)
    {
        for (int x = 0; x < newWidth; ++x) 
        {
            int oldX = newHeight - y - 1;
            int oldY = x;
            int newIndex = y * newRowSize + x;
            int oldIndex = oldY * newRowSize + oldX;
            rotatedBuffer[newIndex] = buffer[oldIndex];
        }
    }

    buffer = rotatedBuffer;
    width = newWidth;
    height = newHeight;

    header.width = width;
    header.height = height;
    header.fileSize = sizeof(BMPHeader) + buffer.size();

    return true;
}

// Вычисление суммы произведений значений пикселей изображения и элементов гауссова ядра для конкретного пикселя
double ImageProcessor::ApplyGaussianKernel(const std::vector<uint8_t>& inputBuffer, const double kernel[3][3], int width, int height, int x, int y)
{
    double sum = 0.0;

    // Проход по 3x3 матрице (гауссовому ядру)
    for (int j = -1; j <= 1; ++j)
    {
        for (int i = -1; i <= 1; ++i)
        {
            // Применение гауссового фильтра к пикселям изображения
            sum += static_cast<double>(inputBuffer[(y + j) * width + (x + i)]) * kernel[j + 1][i + 1];
        }
    }

    return sum;
}

// Применение ядра ко всему изображению, за исключением его краев
void ImageProcessor::ApplyGaussianFilter(std::vector<uint8_t>& outputBuffer, const std::vector<uint8_t>& inputBuffer, int width, int height)
{
    double kernel[3][3] = {
        {1.0 / 16, 2.0 / 16, 1.0 / 16},
        {2.0 / 16, 4.0 / 16, 2.0 / 16},
        {1.0 / 16, 2.0 / 16, 1.0 / 16}
    };

    // Проход по всем пикселям изображения, исключая края
    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            // Применение гауссова ядра к пикселю с координатами (x, y)
            double sum = ApplyGaussianKernel(inputBuffer, kernel, width, height, x, y);
            // Преобразование результата в 8-битное значение и сохранение в выходном буфере
            outputBuffer[y * width + x] = static_cast<uint8_t>(sum);
        }
    }
}

// Метод, применяющий гауссов фильтр к изображению
bool ImageProcessor::ApplyGaussianFilter()
{
    std::vector<uint8_t> filteredBuffer(buffer.size());
    ApplyGaussianFilter(filteredBuffer, buffer, width, height); // Вызов функции для применения гауссова фильтра к изображению
    buffer = filteredBuffer;

    return true;
}

// Метод, сохраняюший изображение в файл с заданным именем
bool ImageProcessor::SaveImage(const std::string& filename)
{
    std::ofstream os(filename, std::ofstream::binary);

    if (!os.is_open())
    {
        std::cerr << "Error creating a file for saving." << std::endl;
        return false;
    }

    BMPHeader updatedHeader = header;
    updatedHeader.width = width;
    updatedHeader.height = height;
    updatedHeader.fileSize = sizeof(BMPHeader) + newRowSize * height;

    os.write(reinterpret_cast<char*>(buffer.data()), sizeof(updatedHeader)); // Заголовок

    os.write(reinterpret_cast<char*>(ImageData.data()), ImageData.size());  // Данные между заголовком и пикселями

    // Перемещение указателя к началу пиксельных данных
    os.seekp(updatedHeader.dataOffset);

    os.write(reinterpret_cast<char*>(buffer.data()), newRowSize * height);

    os.close();

    for (int y = 0; y < header.height; y++) {
        for (int x = 0; x < header.width; x++) {
            uint8_t pixelValue = buffer[y * newRowSize + x];
            std::cout << static_cast<int>(pixelValue) << " ";
        }
        std::cout << std::endl;
    }

    return true;
}


int main()
{
    ImageProcessor image("Picture.bmp");

    if (image.LoadImage())
    {
        // Поворот изображения по часовой стрелке и сохранение результата
        if (image.Rotate1())
        {
            image.SaveImage("Rotated1.bmp");
            std::cout << "Image rotated 90 degrees clockwise and saved successfully." << std::endl;
        }
        else
        {
            std::cerr << "Image rotation 1 failed." << std::endl;
            return 1;
        }

        // Поворот изображения против часовой стрелки и сохранение результата
        if (image.Rotate2())
        {
            image.SaveImage("Rotated2.bmp");
            std::cout << "Image rotated 90 degrees counterclockwise and saved successfully." << std::endl;
        }
        else
        {
            std::cerr << "Image rotation 2 failed." << std::endl;
            return 1;
        }

        // Применение гауссова фильтра к изображению и сохранение результата
        if (image.ApplyGaussianFilter())
        {
            // Сохранение измененного изображения
            image.SaveImage("FilteredImage.bmp");
            std::cout << "Gaussian filter applied and image saved successfully." << std::endl;
        }
        else
        {
            std::cerr << "Gaussian filter application failed." << std::endl;
            return 1;
        }
    }
    else
    {
        std::cerr << "Image loading failed." << std::endl;
        return 1;
    }

    return 0;
}
