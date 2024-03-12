#include "DataSourceController.h"

#include "DataSourceEmulator.h"
#include "DataSourceFile.h"

#include <cstring>
#include <thread>
#include <atomic>

namespace DATA_SOURCE_TASK
{

static std::atomic<bool> is_read_active {true};

std::thread read_thread;

DataSourceController::DataSourceController(
    const std::string & source_path,
    const SOURCE_TYPE & source_type,
    const PAYLOAD_TYPE & p_type,
    const uint32_t & frame_size)
{
    try
    {
        m_buffer = std::make_shared<DataSourceBuffer<std::uint8_t>>(frame_size);

        // Створимо джерело даних
        switch (source_type)
        {
        case SOURCE_TYPE::SOURCE_TYPE_FILE:
            m_data_source = std::make_unique<DataSourceFile>(source_path);
            break;

        case SOURCE_TYPE::SOURCE_TYPE_EMULATOR:
            m_data_source = std::make_unique<DataSourceFileEmulator>(source_type, p_type, frame_size);
            break;
        default:
            break;
        }
    }
    catch (const std::exception & e)
    {
        std::runtime_error(e.what()); // треба використовувати власні обгортки над стандартнимим виключеннями
    }

    m_data_source_frm_processor = std::make_unique<DataSourceFrameProcessor>(frame_size);

    // - організувати зчитування даних в окремому потоці;
    // Потік який читає данні
    read_thread = std::thread(&DataSourceController::readData, this);
}

void DataSourceController::readData()
{
    is_read_active = true;

    static int ret_size = static_cast<int>(DATA_SOURCE_ERROR::READ_SOURCE_ERROR);
    static std::atomic<double> elapsed;

    while (is_read_active)
    {
        Timer timer;
        elapsed = 0;

        // - браковані кадри заповнювати нулями
        memset(m_buffer->payload(), 0, m_buffer->payloadSize());

        // читаємо з джерела
        ret_size = m_data_source->read(m_buffer->data(), m_buffer->size());

        if (ret_size != static_cast<int>(DATA_SOURCE_ERROR::READ_SOURCE_ERROR))
        {
            // обробка даних
            m_data_source_frm_processor->putNewFrame(m_buffer, ret_size);
        }

        elapsed = timer.elapsed();

        // 200 Hz
        while (elapsed < FRAME_RATE)
        {
            elapsed = timer.elapsed();
        }

        m_elapsed = elapsed;
    }
}

DataSourceController::~DataSourceController()
{
    is_read_active = false;

    if (read_thread.joinable())
        read_thread.join();
}

} // namespace DATA_SOURCE_TASK
