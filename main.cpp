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

    bool LoadImage(const std::string& filename);
    bool RotateRight();
    bool RotateLeft();
    bool ApplyGaussianFilter();
    bool SaveImage(const std::string& filename);
private:
    BMPHeader header;
    std::vector<uint8_t> buffer; // Буфер с данными изображения
    std::vector<uint8_t> DataBetween;
    std::vector<uint8_t> WithoutPadding;
    int width;
    int height;
    int padding;
    // Применение гауссова фильтра к изображению
    void ApplyGaussianFilter(std::vector<uint8_t>& outputBuffer, const std::vector<uint8_t>& inputBuffer, int width, int height);
    // Применение гауссового ядра к пикселю изображения
    double ApplyGaussianKernel(const std::vector<uint8_t>& inputBuffer, const double kernel[3][3], int width, int height, int x, int y);
};


ImageProcessor::ImageProcessor(const std::string& filename) 
{
    if (!LoadImage(filename))
    {
        throw std::runtime_error("Error loading the image.");
    }
}

ImageProcessor::~ImageProcessor()
{
    // Деструктор класса ImageProcessor
}

// Метод, загружающий изображение из файла в память
bool ImageProcessor::LoadImage(const std::string& filename)
{
    std::ifstream is(filename, std::ifstream::binary);

    if (!is.is_open())
    {
        std::cerr << "Error opening the file.\n";
        return false;
    }
    is.read(reinterpret_cast<char*>(&header), sizeof(BMPHeader));
    if (header.signature != 0x4D42)
    {
        std::cerr << "The file is not a BMP image.\n";
        is.close();
        return 0;
    }
    if (header.bitsPerPixel != 8)
    {
        std::cerr << "The image should have a color depth of 8 bits.\n";
        is.close();
        return 0;
    }
    width = header.width;
    height = header.height;
    padding = (4 - (width * (header.bitsPerPixel / 8)) % 4) % 4; // Рассчет количества паддинга
    int bufferSize = width * height; // Полный размер буфера
    buffer.resize(bufferSize);
    int dataBetween = header.dataOffset - sizeof(BMPHeader); // Размер данных между заголовком и пиксельными данными
    DataBetween.resize(dataBetween);
    is.read(reinterpret_cast<char*>(DataBetween.data()), dataBetween);
     
    // Чтение данных изображения из файла
    for (int i = 0; i < height; i++)
    {
        is.read(reinterpret_cast<char*>(&buffer[i * width]), width); // Чтение только пиксельных данных

        // Пропуск паддинга в файле
        if (padding > 0) {
            is.seekg(padding, std::ios::cur);  // Сдвиг указателя вперед на количество байтов padding
        }

    }

    is.close();
    std::cout << "Allocated memory size for image loading: " << header.fileSize << " bytes.\n";
    return true;
}

// Метод, выполняющий поворот изображения на 90 градусов по часовой стрелке
bool ImageProcessor::RotateRight()
{
    int newWidth = height;
    int newHeight = width;
    int newPadding = (4 - newWidth % 4) % 4;
    std::vector<uint8_t> rotatedBuffer(newWidth * newHeight);
    
    for (int y = 0; y < newHeight; ++y)
    {
        for (int x = 0; x < newWidth; ++x)
        {
            int oldX = newHeight - y - 1;
            int oldY = x;
            int newIndex = y * newWidth + x;
            int oldIndex = oldY * newHeight + oldX;
            for (int i = 0; i < header.bitsPerPixel / 8; ++i)
            {
                rotatedBuffer[newIndex + i] = buffer[oldIndex + i];
            }
        }
    }

    WithoutPadding.assign(rotatedBuffer.begin(), rotatedBuffer.end());
  
    for (int y = 0; y < newHeight; ++y)
    {
        rotatedBuffer.insert(rotatedBuffer.end(), newPadding, 0);
    }

    buffer = rotatedBuffer;
    width = newWidth;
    height = newHeight;
    header.width = width;
    header.height = height;
    header.fileSize = sizeof(BMPHeader) + DataBetween.size() + buffer.size();

    return true;
}

// Метод, выполняющтй поворот изображения на 90 градусов против часовой стрелки
bool ImageProcessor::RotateLeft()
{
    int newWidth = height;
    int newHeight = width;
    int newPadding = (4 - newWidth % 4) % 4;
    std::vector<uint8_t> rotatedBuffer(newWidth* newHeight);

    for (int y = 0; y < newHeight; ++y)
    {
        for (int x = 0; x < newWidth; ++x)
        {
            int oldX = y;
            int oldY = newWidth - x - 1;
            int newIndex = y * newWidth + x;
            int oldIndex = oldY * newHeight + oldX ;
            for (int i = 0; i < header.bitsPerPixel / 8; ++i)
            {
                rotatedBuffer[newIndex + i] = WithoutPadding[oldIndex + i];
            }
        }
    }

    // Добавдение паддинга
    for (int y = 0; y < newHeight; ++y)
    {
        rotatedBuffer.insert(rotatedBuffer.end(), newPadding, 0);
    }

    buffer = rotatedBuffer;
    width = newWidth;
    height = newHeight;
    header.width = width;
    header.height = height;
    header.fileSize = sizeof(BMPHeader) + DataBetween.size() + buffer.size();

    return true;
}

double ImageProcessor::ApplyGaussianKernel(const std::vector<uint8_t>& inputBuffer, const double kernel[3][3], int width, int height, int x, int y)
{
    double sum = 0.0;
    for (int j = -1; j <= 1; ++j)
    {
        for (int i = -1; i <= 1; ++i)
        {
            int pixelX = x + i;
            int pixelY = y + j;
            // Проверка на края изображения и учет паддинга
            if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height)
            {
                // Применение гауссового фильтра к пикселю изображения
                sum += static_cast<double>(inputBuffer[pixelY * width + pixelX]) * kernel[j + 1][i + 1];
            }
        }
    }
    return sum;
}
void ImageProcessor::ApplyGaussianFilter(std::vector<uint8_t>& outputBuffer, const std::vector<uint8_t>& inputBuffer, int width, int height)
{
    double kernel[3][3] = {
        {1.0 / 16, 2.0 / 16, 1.0 / 16},
        {2.0 / 16, 4.0 / 16, 2.0 / 16},
        {1.0 / 16, 2.0 / 16, 1.0 / 16}
    };
    // Проход по всем пикселям изображения
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // Применение гауссова ядра к пикселю с координатами (x, y)
            double sum = ApplyGaussianKernel(inputBuffer, kernel, width, height, x, y);
            // Преобразование результата в 8-битное значение и сохранение в выходном буфере
            outputBuffer[y * width + x] = static_cast<uint8_t>(sum);
        }
    }
}
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
        std::cerr << "Error creating a file for saving.\n";
        return false;
    }
    BMPHeader updatedHeader = header;
    updatedHeader.width = width;
    updatedHeader.height = height;
    updatedHeader.fileSize = sizeof(BMPHeader) + DataBetween.size() + width * height;

    os.write(reinterpret_cast<char*>(&updatedHeader), sizeof(updatedHeader)); // Заголовок
    os.write(reinterpret_cast<char*>(DataBetween.data()), DataBetween.size());  // Данные между заголовком и пикселями
    os.write(reinterpret_cast<char*>(buffer.data()), width * height);

    os.close();

    return true;
}

int main()
{
    const std::string imageFilename = "Picture.bmp"; 

    ImageProcessor image(imageFilename);
    
    // Поворот изображения по часовой стрелке и сохранение результата
    if (image.RotateRight())
    {
        image.SaveImage("RotateRight.bmp");
        std::cout << "Image rotated 90 degrees clockwise and saved successfully.\n";
    }
    else
    {
        std::cerr << "Image rotatate right failed.\n";
        return 1;
    }

    // Поворот изображения против часовой стрелки и сохранение результата
    if (image.RotateLeft())
    {
        image.SaveImage("RotateLeft.bmp");
        std::cout << "Image rotated 90 degrees counterclockwise and saved successfully.\n";
    }
    else
    {
        std::cerr << "Image rotate left failed.\n";
        return 1;
    }

    // Применение гауссова фильтра к изображению и сохранение результата
    if (image.ApplyGaussianFilter())
    {
        // Сохранение измененного изображения
        image.SaveImage("FilteredImage.bmp");
        std::cout << "Gaussian filter applied and image saved successfully.\n";
    }
    else
    {
        std::cerr << "Gaussian filter application failed.\n";
        return 1;
    }
    
    return 0;
}
