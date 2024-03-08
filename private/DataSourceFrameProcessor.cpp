#include "DataSourceFrameProcessor.h"

#include <cmath>
#include <future>
#include <thread>

namespace DATA_SOURCE_TASK
{

std::thread process_thread;
static std::atomic<bool> is_process_active {true};

inline float validateFloat(const float & falue)
{
    if (std::isfinite(falue))
        return falue;
    else
        return 0.;
}

DataSourceFrameProcessor::DataSourceFrameProcessor(const int & frame_size, const PAYLOAD_TYPE & p_type):
    m_frame_size {frame_size},
    m_packets_loss {-1},
    m_can_validate {false},
    m_src_ready_buffer {-1},
    m_flt_ready_buffer {-1}
{
    // виділимо данні
    switch (p_type)
    {
    case PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT:
    {
        for (std::size_t i = 0; i < MAX_PROCESSING_BUF_NUM; ++i)
            m_source_buffer[i] = std::make_shared<DataSourceBuffer<std::uint8_t>>(frame_size);
    }
    break;
    case PAYLOAD_TYPE::PAYLOAD_TYPE_16_BIT_INT:
    {
        for (std::size_t i = 0; i < MAX_PROCESSING_BUF_NUM; ++i)
            m_source_buffer[i] = std::make_shared<DataSourceBuffer<std::int16_t>>(frame_size);
    }
    break;
    case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_INT:
    {
        for (std::size_t i = 0; i < MAX_PROCESSING_BUF_NUM; ++i)
            m_source_buffer[i] = std::make_shared<DataSourceBuffer<std::int32_t>>(frame_size);
    }
    break;
    case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT:
    {
        for (std::size_t i = 0; i < MAX_PROCESSING_BUF_NUM; ++i)
            m_source_buffer[i] = std::make_shared<DataSourceBuffer<float>>(frame_size);
        break;
    }
    default:
        break;
    }

    // float буфери
    for (std::size_t i = 0; i < MAX_PROCESSING_BUF_NUM; i++)
        m_buffer.push_back(std::make_shared<DataSourceBuffer<float>>(frame_size));

    m_data_source_recorder = std::make_unique<DataSourceFrameRecorder>("record", m_buffer[0]->totalElements());

    process_thread = std::thread(&DataSourceFrameProcessor::frameProcess, this);
}

DataSourceFrameProcessor::~DataSourceFrameProcessor() { is_process_active = false; }

std::future<void> future_result;
void DataSourceFrameProcessor::frameProcess()
{
    static Timer timer;

    // індекс буферу обробки.
    static std::atomic<int> idx;
    idx = 0;

    while (is_process_active)
    {
        if (m_can_validate)
        {
            if (std::abs(idx - m_src_ready_buffer) < 1) // поточний індекс буфера обробки мусить відставати
                continue;

            timer.reset();

            if (validateFrame(m_source_buffer[idx], m_req_size))
            {
                // реєстрація блоків даних
                m_data_source_recorder->putNewFrame(m_buffer[m_flt_ready_buffer]);
            }

            if (++idx >= static_cast<int>(MAX_PROCESSING_BUF_NUM))
                idx = 0;

            m_elapsed = timer.elapsed();
        }
    }
}

bool DataSourceFrameProcessor::validateFrame(
    std::shared_ptr<DataSourceBufferInterface> & buffer, const int & updated_size)
{
    std::lock_guard<std::mutex> lock(m_process_mutex);

    frame * frm    = buffer->frame();
    char * payload = buffer->payload();

    static int cur_frm_counter = -1;

    if (cur_frm_counter == -1)
    {
        cur_frm_counter = frm->frame_counter;
        m_packets_loss  = 0;
    }
    else
    {
        // лічільник кадрів
        const int delta = frm->frame_counter - cur_frm_counter;

        if (delta > 1)
        {
            m_packets_loss += frm->frame_counter - cur_frm_counter - 1;
        }
    }

    cur_frm_counter = frm->frame_counter;

    // розмір не відповідає необхідному - втрачений кадр?
    if (updated_size != frameSize())
    {
        ++m_packets_loss;
    }

    // Готовий буфер для запису
    ++m_flt_ready_buffer;

    if (m_flt_ready_buffer >= static_cast<int>(MAX_PROCESSING_BUF_NUM))
        m_flt_ready_buffer = 0;

    DataSourceBuffer<float> * cur_buf = m_buffer[m_flt_ready_buffer].get();

    // оновимо заголовок
    std::copy(frm, frm + FRAME_HEADER_SIZE, cur_buf->frame());

    // - реалізувати максимально обчислювально ефективне перетворення усіх даних
    // до єдиного типу 32 bit IEEE 754 float та приведення до діапазону +/-1.0;
    if (frm->payload_type != PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT)
    {
        // CPU
        for (std::uint32_t i = 0; i < cur_buf->totalElements(); ++i)
        {
            cur_buf->payload()[i] = validateFloat(static_cast<float>(payload[i]));
        }

        return true;
    }

    // перекладемо дані якшо вони вже в форматі float
    std::copy(payload, payload + cur_buf->payloadSize(), cur_buf->payload());

    return true;
}

void DataSourceFrameProcessor::putNewFrame(std::shared_ptr<DataSourceBufferInterface> & buffer, const int & updated_size)
{
    std::lock_guard<std::mutex> lock(m_process_mutex);

    // Готовий буфер для запису
    ++m_src_ready_buffer;

    if (m_src_ready_buffer >= static_cast<int>(MAX_PROCESSING_BUF_NUM))
        m_src_ready_buffer = 0;

    // Обміняємо буфери для обробки
    m_source_buffer[m_src_ready_buffer].swap(buffer);

    m_req_size = updated_size;
    m_can_validate = true;
}

} // namespace DATA_SOURCE_TASK
