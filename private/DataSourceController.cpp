#include "DataSourceController.h"

#include "globals.h"

#include <cstring>
#include <thread>

namespace DATA_SOURCE_TASK
{

DataSourceController::DataSourceController(const std::shared_ptr<DataSource> & data_source, const uint32_t & frame_size):
    m_data_source {data_source}
{
    m_buffer = std::make_shared<DataSourceBuffer<std::uint8_t>>(frame_size);

    // Обробка кадрів - перетворенн в float, складання, запис
    m_data_source_frm_processor = std::make_unique<DataSourceFrameProcessor>(frame_size);

    // - організувати зчитування даних в окремому потоці;
    // Потік який читає данні
    m_read_thread = std::thread(&DataSourceController::readData, this);
}

void DataSourceController::readData()
{
    m_is_read_active = true;

    static int ret_size = static_cast<int>(DATA_SOURCE_ERROR::READ_SOURCE_ERROR);
    static std::atomic<double> elapsed;

    static Timer timer;

    while (m_is_read_active)
    {
        timer.reset();
        elapsed = 0;

        // - браковані кадри заповнювати нулями
        memset(m_buffer->payload(), 0, m_buffer->payloadSize());

        // читаємо з джерела
        ret_size = m_data_source->read(m_buffer->data(), m_buffer->size());

        if (ret_size > 0)
        {
            // обробка даних
            m_data_source_frm_processor->putNewFrame(m_buffer, ret_size);
        }

        elapsed = timer.elapsed();

        // 200 Hz
        while (elapsed < MAX_FREQ_READ)
        {
            elapsed = timer.elapsed();
        }

        m_elapsed = elapsed;
    }
}

DataSourceController::~DataSourceController()
{
    m_is_read_active = false;

    if (m_read_thread.joinable())
        m_read_thread.join();
}

} // namespace DATA_SOURCE_TASK
