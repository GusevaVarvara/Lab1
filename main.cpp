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
    int width;
    int height;
    int rowSize;

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

    if (!is)
    {
        std::cerr << "Error opening the file." << std::endl;
        return 1;
    }

    is.seekg(0, is.end);
    int length = is.tellg();
    is.seekg(0, is.beg);

    std::cout << "Allocated memory size for image loading: " << length << " bytes" << std::endl;

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

    int rowSize = ((width * header.bitsPerPixel + 31) / 32) * 4; // Рассчет размера строки с учетом паддинга
    int padding = rowSize - ((width * header.bitsPerPixel) / 8);  // Рассчет количества паддинга
    int bufferSize = rowSize * height; // Полный размер буфера

    buffer.resize(bufferSize);

    is.seekg(header.dataOffset); // Перемещение указателя в файле к данным изображения

    // Чтение данных изображения из файла
    for (int i = 0; i < height; i++)
    {
        is.read(reinterpret_cast<char*>(&buffer[i * rowSize]), rowSize);
        is.seekg(padding, std::ios::cur); // Пропускаю паддинг
    }

    is.close();

    return true;
}

// Метод, выполняющий поворот изображения на 90 градусов по часовой стрелке
bool ImageProcessor::Rotate1()
{
    int newWidth = height;
    int newHeight = width;
    std::vector<uint8_t> rotatedBuffer(buffer.size());

    int newRowSize = ((newWidth * header.bitsPerPixel + 31) / 32) * 4; // Новый размер строки с учетом паддинга

    for (int y = 0; y < newHeight; ++y)
    {
        for (int x = 0; x < newWidth; ++x)
        {
            int oldX = y;
            int oldY = newWidth - x - 1;
            int newIndex = x * newRowSize + y;
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

    for (int y = 0; y < newHeight; ++y)
    {
        for (int x = 0; x < newWidth; ++x)
        {
            int oldX = newHeight - y - 1;
            int oldY = x;
            int newIndex = x * newHeight + y;
            int oldIndex = oldY * width + oldX;
            rotatedBuffer[newIndex] = buffer[oldIndex];
        }
    }

    buffer = rotatedBuffer;
    width = newWidth;
    height = newHeight;

    // Обновление заголовка BMP
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

    if (!os)
    {
        std::cerr << "Error creating a file for saving." << std::endl;
        return false;
    }

    header.width = width;
    header.height = height;
    header.fileSize = sizeof(BMPHeader) + buffer.size();

    os.write(reinterpret_cast<char*>(&header), sizeof(BMPHeader));

    // Рассчет размера строки с учетом паддинга
    int rowSize = ((width * header.bitsPerPixel + 31) / 32) * 4;
    // Рассчет количества паддинга
    int padding = rowSize - ((width * header.bitsPerPixel) / 8);

    // Создание буфера для заполнения пробела между заголовком и данными изображения
    std::vector<uint8_t> gapData(header.dataOffset - sizeof(BMPHeader), 0);
    os.write(reinterpret_cast<char*>(gapData.data()), gapData.size());

    os.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
    os.close();

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
