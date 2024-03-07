#include "DataSourceController.h"
#include "DataSourceFileEmulator.h"

#include <chrono>
#include <iostream>
#include <ostream>
#include <thread>

static constexpr const char * FILE_SOURCE = "dev";

// 10 МБ/с = 10×1024×1024 байт/с - мінімальна пропускна здатність
// 100 МБ/с = 100×1024×1024 байт/с - максимальна пропускна здатність
// 200 Гц - частота видачі кадрів

// Максимальний розмір кадру = (100×1024×1024) / 200 = 512 * 1024 Байт
static constexpr int MAX_FRAME_SIZE {((512 * 1024))};

// Мінімальний розмір кадру = (10×1024×1024) / 200 = 51 * 1024 Байт
static constexpr int MIN_FRAME_SIZE {((51 * 1024))};

#ifdef _WIN32
// For Windows
#define CLEAR_CONSOLE "cls"
#else
// For Unix-like systems (Linux, macOS)
#define CLEAR_CONSOLE "clear"
#endif

int main(int argc, char ** argv)
{
    static_cast<void>(argc);
    static_cast<void>(argv);

    // Обмін через файл FILE_SOURCE
    constexpr DATA_SOURCE_TASK::SOURCE_TYPE s_type {DATA_SOURCE_TASK::SOURCE_TYPE::SOURCE_TYPE_EMULATOR};

    // Джерело повинно записувати числа з плаваючою крапкою
    constexpr DATA_SOURCE_TASK::PAYLOAD_TYPE p_type {DATA_SOURCE_TASK::PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT};

    try
    {
#if 0
        // Емулятор джерела даних
        std::unique_ptr<DataSourceFileEmulator> file_data_source_emulator
            = std::make_unique<DataSourceFileEmulator>(FILE_SOURCE, s_type, p_type, MAX_FRAME_SIZE);
#endif

        // Обробка фреймів з джерела
        std::unique_ptr<DATA_SOURCE_TASK::DataSourceController> data_source_processor
            = std::make_unique<DATA_SOURCE_TASK::DataSourceController>(FILE_SOURCE, s_type, p_type, MAX_FRAME_SIZE);

        DATA_SOURCE_TASK::Timer display_update_timer;
        // Вивід результатів в консоль
        for (;;)
        {
            display_update_timer.reset();
            // Читимо консоль перед нови виводом даних
            system(CLEAR_CONSOLE);

            static int prev_counter = -1;

            int diff_frames = data_source_processor->framesTotal() - (prev_counter == -1 ? 0 : prev_counter);

            std::cout << "Frame size: " << MAX_FRAME_SIZE << std::endl;
            std::cout << "Elapsed time for frame write: " << data_source_processor->dataSource().elapsed() << std::endl;
            std::cout << "Elapsed time for frame read: " << data_source_processor->elapsed() << std::endl;

            // Заголовок, к-сть обробленних кадрів, к-сть втрачених
            std::cout << "Frames recieved: " << data_source_processor->framesTotal() << std::endl;
            std::cout << "Processed frames: " << diff_frames<< std::endl;
            std::cout << "Frame head: " << std::hex << data_source_processor->header() << std::dec << std::endl;
            std::cout << "Frames loss: " << data_source_processor->getPacketsLoss() << std::endl;
            std::cout << "Frames loss %: " << (100. * data_source_processor->getPacketsLoss()) / data_source_processor->framesTotal() <<"%"<< std::endl;

            std::cout << "Elapsed time for frame process: " << data_source_processor->frameProcessor().elapsed()
                      << std::endl;

            // пауза перед оновленням виводу
            prev_counter = data_source_processor->framesTotal();

            while (display_update_timer.elapsed() < 1000) {

            }
        }
    }
    catch (const std::exception & ex)
    {
        std::cerr << "An exceptio occured: " << ex.what() << std::endl;
        return -1;
    }

    return 0;
}
