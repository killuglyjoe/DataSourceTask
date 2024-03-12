#include "DataSourceController.h"

#include <iostream>
#include <ostream>

static constexpr const char * FILE_SOURCE = "dev";

// 10 МБ/с = 10×1024×1024 байт/с - мінімальна пропускна здатність
// 100 МБ/с = 100×1024×1024 байт/с - максимальна пропускна здатність
// 200 Гц - частота видачі кадрів

// Максимальний розмір кадру = (100×1024×1024) / 200 = 512 * 1024 Байт
static constexpr int MAX_FRAME_SIZE {512 * 1024};

// Мінімальний розмір кадру = (10×1024×1024) / 200 = 51 * 1024 Байт
static constexpr int MIN_FRAME_SIZE {51 * 1024};

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

    // Тип джерела.
    constexpr DATA_SOURCE_TASK::SOURCE_TYPE s_type {DATA_SOURCE_TASK::SOURCE_TYPE::SOURCE_TYPE_EMULATOR};

    // Тип вх. даних.
    constexpr DATA_SOURCE_TASK::PAYLOAD_TYPE p_type {DATA_SOURCE_TASK::PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT};

    try
    {
        // Клас контролер для різних типів даних з різних джерел.
        std::unique_ptr<DATA_SOURCE_TASK::DataSourceController> data_source_processor
            = std::make_unique<DATA_SOURCE_TASK::DataSourceController>(FILE_SOURCE, s_type, p_type, MAX_FRAME_SIZE);

        // Таймер оновлення виводу в консоль
        DATA_SOURCE_TASK::Timer display_update_timer;
        // Вивід результатів в консоль
        for (;;)
        {
            display_update_timer.reset();

            // Читимо консоль перед новим виводом даних
            system(CLEAR_CONSOLE);

            // грубий лічильник оброблених кадрів
            static int prev_counter = -1;

            int diff_frames = data_source_processor->framesTotal() - (prev_counter == -1 ? 0 : prev_counter);

            std::cout << "Frame size: " << MAX_FRAME_SIZE << " bytes"<< std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;
            std::cout << "Elapsed time for frame write: " << data_source_processor->writeFramelapsed() << " ms" << std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;
            std::cout << "Elapsed time for frame read: " << data_source_processor->elapsed() << " ms" << std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;

            // Заголовок, к-сть обробленних кадрів, к-сть втрачених і відсоток втрачених кадрів за ~1сек
            std::cout << "Frame head: " << std::hex << data_source_processor->header() << std::dec << std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;
            std::cout << "Frames recieved: " << data_source_processor->framesTotal() << std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;
            std::cout << "Processed frames: " << diff_frames << std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;
            std::cout << "Bad frames: " << data_source_processor->getBadFrames() << std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;
            std::cout << "Frames loss: " << data_source_processor->getPacketsLoss() << std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;
            std::cout << "Percentage loss: " << (100. * data_source_processor->getPacketsLoss()) / data_source_processor->framesTotal() << " %" << std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;

            // Груба частота кадрів, Мб/сек
            std::cout << "Download speed: " << (diff_frames * MAX_FRAME_SIZE) / (1000. * 1000.) << " Mb/sec" << std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;

            // Час перетворення на float
            std::cout << "Elapsed time for frame validation: " << data_source_processor->frameValidationElapsed() << " ms" << std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;
            // Час запису в файл
            std::cout << "Elapsed time for frame record: " << data_source_processor->saveFramelapsed() << " ms" << std::endl;
            std::cout << "-----------------------------------------------"<< std::endl;

            prev_counter = data_source_processor->framesTotal();

            // пауза перед оновленням екрану
            while (display_update_timer.elapsed() < 1000.)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
