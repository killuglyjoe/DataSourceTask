#include "DataSourceController.h"
#include "DataSourceEmulator.h"

#include <signal.h>

#include <iostream>
#include <sstream>
#include <ostream>

// 10 МБ/с = 1250000 байт/с - мінімальна пропускна здатність
// 100 МБ/с = 12500000 байт/с - максимальна пропускна здатність
// 200 Гц - частота видачі кадрів

// Максимальний розмір кадру = (100 000 000 / 8) / 200 = 62 500 Байт
static constexpr int MAX_FRAME_SIZE {static_cast<int>((12500000) / DATA_SOURCE_TASK::FRAME_RATE_PER_SEC)};

// Мінімальний розмір кадру = (10×1024×1024) / 200 = 51 * 1024 Байт
// static constexpr int MIN_FRAME_SIZE {static_cast<int>((1250000) / DATA_SOURCE_TASK::FRAME_RATE_PER_SEC)};

#ifdef _WIN32
// For Windows
#define CLEAR_CONSOLE "cls"
#else
// For Unix-like systems (Linux, macOS)
#define CLEAR_CONSOLE "clear"
#endif

bool g_main_loop {true};

void exit_handler(int s)
{
    std::cout << "Signal caught - " << s << std::endl;
    g_main_loop = false;
}

int main(int argc, char ** argv)
{
    static_cast<void>(argc);
    static_cast<void>(argv);

    signal(SIGINT, &exit_handler);

    // Тип вх. даних.
    constexpr DATA_SOURCE_TASK::PAYLOAD_TYPE p_type {DATA_SOURCE_TASK::PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT};

    std::shared_ptr<DATA_SOURCE_TASK::DataSource> data_source;
    try
    {
        data_source = std::make_unique<DATA_SOURCE_TASK::DataSourceFileEmulator>(p_type, MAX_FRAME_SIZE);

        // Клас контролер для різних типів даних з різних джерел.
        std::unique_ptr<DATA_SOURCE_TASK::DataSourceController> data_source_processor
            = std::make_unique<DATA_SOURCE_TASK::DataSourceController>(data_source, MAX_FRAME_SIZE);

        // Таймер оновлення виводу в консоль
        DATA_SOURCE_TASK::Timer display_update_timer;

        // Вивід результатів в консоль
        while (g_main_loop)
        {
            display_update_timer.reset();

            // Читимо консоль перед новим виводом даних
            system(CLEAR_CONSOLE);

            // грубий лічильник оброблених кадрів
            static int prev_counter = -1;

            int diff_frames = data_source_processor->framesTotal() - (prev_counter == -1 ? 0 : prev_counter);

            static std::stringstream ss;
            ss.clear();

            ss << "Frame size: " << MAX_FRAME_SIZE << " bytes\n";
            ss << "-----------------------------------------------\n";
            ss << "Elapsed time for frame write: " << data_source_processor->writeFramelapsed() << " ms\n";
            ss << "-----------------------------------------------\n";
            ss << "Elapsed time for frame read: " << data_source_processor->elapsed() << " ms\n";
            ss << "-----------------------------------------------\n";

            // Заголовок, к-сть обробленних кадрів, к-сть втрачених і відсоток втрачених кадрів за ~1сек
            ss << "Frame head: " << std::hex << data_source_processor->header() << std::dec << "\n";
            ss << "-----------------------------------------------\n";
            ss << "Frames recieved: " << data_source_processor->framesTotal() << "\n";
            ss << "-----------------------------------------------\n";
            ss << "Processed frames: " << diff_frames << "\n";
            ss << "-----------------------------------------------\n";
            ss << "Bad frames: " << data_source_processor->getBadFrames() << "\n";
            ss << "-----------------------------------------------\n";
            ss << "Frames loss: " << data_source_processor->getPacketsLoss() << "\n";
            ss << "-----------------------------------------------\n";
            ss << "Broken stream frames: " << data_source_processor->getBrokenFrames() << "\n";
            ss << "-----------------------------------------------\n";
            ss << "Percentage loss: "
               << (100. * data_source_processor->getPacketsLoss()) / data_source_processor->framesTotal() << " %\n";
            ss << "-----------------------------------------------\n";

            // Груба частота кадрів, Мб/сек
            static constexpr float byte_sec_2_bit_sec_conv {8. / (1000. * 1000.)};
            ss << "Download speed: " << (diff_frames * MAX_FRAME_SIZE) * byte_sec_2_bit_sec_conv << " Mb/sec\n";
            ss << "-----------------------------------------------\n";

            // Час перетворення на float
            ss << "Elapsed time for frame validation: " << data_source_processor->frameValidationElapsed() << " ms\n";
            ss << "-----------------------------------------------\n";
            // Час запису в файл
            ss << "Elapsed time for frame record: " << data_source_processor->saveFramelapsed() << " ms\n";
            ss << "-----------------------------------------------\n";

            prev_counter = data_source_processor->framesTotal();

            std::cout << ss.rdbuf() << std::endl;

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
