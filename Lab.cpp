#include <iostream>
#include <fstream>
#include <windows.h>

using namespace std;

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

// Функция для применения фильтра Гаусса к изображению
void ApplyGaussianFilter(uint8_t* inputBuffer, uint8_t* outputBuffer, int width, int height)
{
    double kernel[3][3] =
    {
        {1.0 / 16, 2.0 / 16, 1.0 / 16},
        {2.0 / 16, 4.0 / 16, 2.0 / 16},
        {1.0 / 16, 2.0 / 16, 1.0 / 16}
    };

    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            double sum = 0.0;
            for (int j = -1; j <= 1; ++j)
            {
                for (int i = -1; i <= 1; ++i)
                {
                    sum += static_cast<double>(inputBuffer[(y + j) * width + (x + i)]) * kernel[j + 1][i + 1];
                }
            }
            // Записываю результат в выходной буфер
            outputBuffer[y * width + x] = static_cast<uint8_t>(sum);
        }
    }
}

int main()
{
    SetConsoleOutputCP(1251);

    ifstream is("Picture.bmp", ifstream::binary);
    if (!is)
    {
        cerr << "Ошибка при открытии файла." << endl; 
        return 1;
    }

    is.seekg(0, is.end);
    int length = is.tellg();
    is.seekg(0, is.beg);

    cout << "Количество выделяемой памяти для загрузки изображения: " << length << " байт" << endl;

    // Считываю заголовок файла
    BMPHeader header;
    is.read(reinterpret_cast<char*>(&header), sizeof(BMPHeader));

    // Проверяю файл
    if (header.signature != 0x4D42)
    {
        cerr << "Файл не является BMP изображением." << endl;
        is.close();
        return 1;
    }

    if (header.bitsPerPixel != 8)
    {
        cerr << "Изображение должно иметь глубину цвета 8 бит." << endl;
        is.close();
        return 1;
    }

    // Размер изображения
    int width = header.width;
    int height = header.height;

    // Размер буфера для пикселей
    int bufferSize = width * height;
    uint8_t* buffer = new uint8_t[bufferSize];

    // Читаю пиксели из исходного файла
    is.seekg(header.dataOffset);
    is.read(reinterpret_cast<char*>(buffer), bufferSize);

    is.close();

    // Создаю новые буферы для повернутых изображений и для изображения с фильтром 
    int newWidth = height;
    int newHeight = width;
    int newBufferSize = newWidth * newHeight;
    uint8_t* reverse1Buffer = new uint8_t[newBufferSize];
    uint8_t* reverse2Buffer = new uint8_t[newBufferSize];
    uint8_t* gaussianFilteredBuffer = new uint8_t[newBufferSize];

    // Поворачиваю изображение на 90 градусов по часовой
    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            int newIndex = (width - j - 1) * newWidth + i;
            int oldIndex = i * width + j;
            reverse1Buffer[newIndex] = buffer[oldIndex];
        }
    }

    // Поворачиваю изображение на 90 градусов против часовой
    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            int newIndex = j * newWidth + (height - i - 1);
            int oldIndex = i * width + j;
            reverse2Buffer[newIndex] = buffer[oldIndex];
        }
    }

    // Файл с поворотом изображения по часовой стрелке
    ofstream os("Rotated1.bmp", ofstream::binary);
    if (!os)
    {
        cerr << "Ошибка при создании файла для сохранения." << endl;
        delete[] buffer;
        delete[] reverse1Buffer;
        delete[] reverse2Buffer;
        return 1;
    }

    os.write(reinterpret_cast<char*>(&header), sizeof(BMPHeader));

    // Обновляю ширину и высоту в заголовке на новые значения
    header.width = newWidth;
    header.height = newHeight;
    os.seekp(18);  
    os.write(reinterpret_cast<char*>(&newWidth), sizeof(newWidth));
    os.write(reinterpret_cast<char*>(&newHeight), sizeof(newHeight));

    // Записываю повернутые пиксели в файл
    os.write(reinterpret_cast<char*>(reverse1Buffer), newBufferSize);
    os.close();

    // Обновляю размер файла в заголовке
    ifstream checkSize("Rotated1.bmp", ifstream::binary);
    if (!checkSize)
    {
        cerr << "Ошибка при проверке размера файла." << endl;
        delete[] buffer;
        delete[] reverse1Buffer;
        delete[] reverse2Buffer;
        return 1;
    }

    checkSize.seekg(0, checkSize.end);
    int fileSize = checkSize.tellg();
    checkSize.close();

    header.fileSize = fileSize;

    // Обновление заголовка
    ofstream updateHeader("Rotated1.bmp", ofstream::binary | ofstream::in);
    if (!updateHeader)
    {
        cerr << "Ошибка при обновлении заголовка файла." << endl;
        delete[] buffer;
        delete[] reverse1Buffer;
        delete[] reverse2Buffer;
        return 1;
    }

    // Обновляю fileSize в заголовке 
    updateHeader.seekp(2);
    updateHeader.write(reinterpret_cast<char*>(&header.fileSize), sizeof(header.fileSize));
    updateHeader.close();

    cout << "Изображение успешно повернуто на 90 градусов по часовой стрелке и сохранено в файл Rotated1.bmp." << endl;

    // Файл с поворотом изображения против часовой стрелки
    ofstream os2("Rotated2.bmp", ofstream::binary);
    if (!os2)
    {
        cerr << "Ошибка при создании файла для сохранения второго изображения." << endl;
        delete[] buffer;
        delete[] reverse1Buffer;
        delete[] reverse2Buffer;
        return 1;
    }

    os2.write(reinterpret_cast<char*>(&header), sizeof(BMPHeader));

    header.width = newWidth;
    header.height = newHeight;
    os2.seekp(18);
    os2.write(reinterpret_cast<char*>(&newWidth), sizeof(newWidth));
    os2.write(reinterpret_cast<char*>(&newHeight), sizeof(newHeight));

    os2.write(reinterpret_cast<char*>(reverse2Buffer), newBufferSize);
    os2.close();

    ifstream checkSize2("Rotated2.bmp", ifstream::binary);
    if (!checkSize2)
    {
        cerr << "Ошибка при проверке размера второго файла." << endl;
        delete[] buffer;
        delete[] reverse1Buffer;
        delete[] reverse2Buffer;
        return 1;
    }

    checkSize2.seekg(0, checkSize2.end);
    fileSize = checkSize2.tellg();
    checkSize2.close();

    header.fileSize = fileSize;

    ofstream updateHeader2("Rotated2.bmp", ofstream::binary | ofstream::in);
    if (!updateHeader2)
    {
        cerr << "Ошибка при обновлении заголовка второго файла." << endl;
        delete[] buffer;
        delete[] reverse1Buffer;
        delete[] reverse2Buffer;
        return 1;
    }

    updateHeader2.seekp(2);
    updateHeader2.write(reinterpret_cast<char*>(&header.fileSize), sizeof(header.fileSize));
    updateHeader2.close();

    cout << "Изображение успешно повернуто на 90 градусов против часовой стрелки и сохранено в файл Rotated2.bmp." << endl;

    // Применяю фильтр Гаусса к первому повернутому изображению
    ApplyGaussianFilter(reverse1Buffer, gaussianFilteredBuffer, newWidth, newHeight);

    // Файл с применением фильтра Гаусса
    ofstream os3("FilteredRotated1.bmp", ofstream::binary);
    if (!os3)
    {
        cerr << "Ошибка при создании файла для сохранения фильтрованного изображения." << endl;
        delete[] buffer;
        delete[] reverse1Buffer;
        delete[] reverse2Buffer;
        delete[] gaussianFilteredBuffer;
        return 1;
    }

    os3.write(reinterpret_cast<char*>(&header), sizeof(BMPHeader));

    header.width = newWidth;
    header.height = newHeight;
    os3.seekp(18);
    os3.write(reinterpret_cast<char*>(&newWidth), sizeof(newWidth));
    os3.write(reinterpret_cast<char*>(&newHeight), sizeof(newHeight));

    // Записываю фильтрованные пиксели в файл
    os3.write(reinterpret_cast<char*>(gaussianFilteredBuffer), newBufferSize);
    os3.close();

    // Обновляю размер файла в заголовке
    ifstream checkSize3("FilteredRotated1.bmp", ifstream::binary);
    if (!checkSize3)
    {
        cerr << "Ошибка при проверке размера фильтрованного файла." << endl;
        delete[] buffer;
        delete[] reverse1Buffer;
        delete[] reverse2Buffer;
        delete[] gaussianFilteredBuffer;
        return 1;
    }

    checkSize3.seekg(0, checkSize3.end);
    fileSize = checkSize3.tellg();
    checkSize3.close();

    header.fileSize = fileSize;

    ofstream updateHeader3("FilteredRotated1.bmp", ofstream::binary | ofstream::in);
    if (!updateHeader3)
    {
        cerr << "Ошибка при обновлении заголовка фильтрованного файла." << endl;
        delete[] buffer;
        delete[] reverse1Buffer;
        delete[] reverse2Buffer;
        delete[] gaussianFilteredBuffer;
        return 1;
    }

    updateHeader3.seekp(2);
    updateHeader3.write(reinterpret_cast<char*>(&header.fileSize), sizeof(header.fileSize));
    updateHeader3.close();

    cout << "Применен фильтр Гаусса и сохранен в файл FilteredRotated1.bmp." << endl;

    // Освобождаю память
    delete[] buffer;
    delete[] reverse1Buffer;
    delete[] reverse2Buffer;
    delete[] gaussianFilteredBuffer;

    return 0;
}



