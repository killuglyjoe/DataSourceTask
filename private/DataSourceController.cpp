#include "DataSourceController.h"

#include "DataSourceFile.h"

#include <cstring>
#include <future>
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
    const int & num_elements):
    m_active_buffer {0}
{
    try
    {
        // Створимо джерело даних
        switch (source_type)
        {
        case SOURCE_TYPE::SOURCE_TYPE_FILE:
            m_data_source = std::make_unique<DataSourceFile>(source_path);
            break;
        default:
            break;
        }

        // виділимо данні
        switch (p_type)
        {
        case PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT:
            m_byte_size = num_elements * sizeof(std::uint8_t);
            for (std::size_t i = 0; i < MAX_READ_BUF_NUM; ++i)
                m_buffer[i] = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<std::uint8_t>>(num_elements);
            break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_16_BIT_INT:
            m_byte_size = num_elements * sizeof(std::int16_t);
            for (std::size_t i = 0; i < MAX_READ_BUF_NUM; ++i)
                m_buffer[i] = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<std::int16_t>>(num_elements);
            break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_INT:
            m_byte_size = num_elements * sizeof(std::int32_t);
            for (std::size_t i = 0; i < MAX_READ_BUF_NUM; ++i)
                m_buffer[i] = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<std::int32_t>>(num_elements);
            break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT:
            m_byte_size = num_elements * sizeof(float);
            for (std::size_t i = 0; i < MAX_READ_BUF_NUM; ++i)
                m_buffer[i] = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<float>>(num_elements);
            break;
        default:
            break;
        }
    }
    catch (const std::exception & e)
    {
        std::runtime_error(e.what()); // треба використовувати власні обгортки над стандартнимим виключеннями
    }

    m_data_source_frm_processor = std::make_unique<DataSourceFrameProcessor>(m_buffer[0]->size(), num_elements);

    m_data_source_recorder = std::make_unique<DataSourceFrameRecorder>("record", num_elements);

    // - організувати зчитування даних в окремому потоці;
    // Потік який читає данні
    read_thread = std::thread(&DataSourceController::readData, this);
}

void DataSourceController::readData()
{
    is_read_active = true;

    std::future<void> future_result;

    while (is_read_active)
    {
        static int ret_size = 0;

        // - браковані кадри заповнювати нулями
        memset(m_buffer[m_active_buffer]->payload(), 0, m_byte_size);

        // читаємо з джерела
        ret_size = m_data_source->read(m_buffer[m_active_buffer]->data(), m_buffer[m_active_buffer]->size());

        if (ret_size != static_cast<int>(DATA_SOURCE_ERROR::READ_SOURCE_ERROR))
        {
            int buf_num   = m_active_buffer;

            future_result = std::async(
                std::launch::async,
                [&](const int & ret_size, const int & buf_num)
                {
                    // обробка даних
                    if (m_data_source_frm_processor->validateFrame(m_buffer[buf_num], ret_size))
                    {
                        // реєстрація блоків даних
                        m_data_source_recorder->putNewFrame(*m_data_source_frm_processor->curProcessedFrame());
                    }
                    else
                    {

                    }
                },
                ret_size,
                buf_num);
        }

        ++m_active_buffer;
        if (m_active_buffer == MAX_READ_BUF_NUM)
            m_active_buffer = 0;
    }
}

DataSourceController::~DataSourceController()
{
    is_read_active = false;

    if (read_thread.joinable())
        read_thread.join();
}

} // namespace DATA_SOURCE_TASK
