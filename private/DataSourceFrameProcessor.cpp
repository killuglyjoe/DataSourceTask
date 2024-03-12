#include "DataSourceFrameProcessor.h"

#include <cmath>
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

DataSourceFrameProcessor::DataSourceFrameProcessor(const int & frame_size,
                                                   const PAYLOAD_TYPE & p_type):
    m_frame_size {frame_size},
    m_packets_loss {0},
    m_bad_frames {0},
    m_can_validate {false},
    m_src_ready_buffer {-1},
    m_active_buffer {0},
    m_flt_ready_buffer {-1}
{
    // виділимо данні
    for (std::size_t b = 0; b < BUFERIZATION_NUM; ++b)
    {
        for (std::size_t i = 0; i < MAX_PROCESSING_BUF_NUM; ++i)
        {
            m_source_buffer[b][i] = std::make_shared<DataSourceBuffer<std::uint8_t>>(frame_size);
        }
    }

    // float буфери
    for (std::size_t i = 0; i < MAX_PROCESSING_BUF_NUM; i++)
    {
        m_buffer.push_back(std::make_shared<DataSourceBuffer<float>>(frame_size));
    }

    m_data_source_recorder = std::make_unique<DataSourceFrameRecorder>("record", m_buffer[0]->totalElements());

    process_thread = std::thread(&DataSourceFrameProcessor::frameProcess, this);
}

DataSourceFrameProcessor::~DataSourceFrameProcessor() { is_process_active = false; }

void DataSourceFrameProcessor::frameProcess()
{
    static Timer timer;

    while (is_process_active)
    {
        if (m_can_validate)
        {
            timer.reset();

            for (std::size_t idx = 0; idx < MAX_PROCESSING_BUF_NUM; ++idx)
            {
                bool is_validated = false;

                // поточний буфер оновлюється, тому беремо попередній.
                int ready_buffer = m_active_buffer - 1;

                if (ready_buffer < 0)
                {
                    ready_buffer = BUFERIZATION_NUM - 1;
                }

                is_validated = validateFrame(m_source_buffer[ready_buffer][idx]);

                if (is_validated)
                {
                    // реєстрація блоків даних
                    m_data_source_recorder->putNewFrame(m_buffer[m_flt_ready_buffer]);
                }
            }

            m_can_validate = false;
            m_elapsed      = timer.elapsed();
        }
    }
}

bool DataSourceFrameProcessor::validateFrame(std::shared_ptr<DataSourceBufferInterface> & buffer)
{
    std::lock_guard<std::mutex> lock(m_process_mutex);

    frame * frm    = buffer->frame();
    char * buf     = buffer->payload();

    static int cur_frm_counter = -1;

    if (cur_frm_counter == -1)
    {
        cur_frm_counter = frm->frame_counter;
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

    if (m_flt_ready_buffer >= static_cast<int>(MAX_PROCESSING_BUF_NUM) - 1)
        m_flt_ready_buffer = -1;

    // Готовий буфер для запису
    ++m_flt_ready_buffer;

    DataSourceBuffer<float> * cur_buf = m_buffer[m_flt_ready_buffer].get();

    // оновимо заголовок
    std::copy(frm, frm + FRAME_HEADER_SIZE, cur_buf->frame());

    // - реалізувати максимально обчислювально ефективне перетворення усіх даних
    // до єдиного типу 32 bit IEEE 754 float та приведення до діапазону +/-1.0;
    if (frm->payload_type != PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT)
    {
        for (std::uint32_t i = 0; i < cur_buf->totalElements(); ++i)
        {
            switch (frm->payload_type)
            {
            case PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT:
            {
                std::uint8_t * payload = reinterpret_cast<std::uint8_t *>(buf);
                cur_buf->payload()[i]  = validateFloat(static_cast<float>(payload[i]));
            }
            break;
            case PAYLOAD_TYPE::PAYLOAD_TYPE_16_BIT_INT:
            {
                std::int16_t * payload = reinterpret_cast<std::int16_t *>(buf);
                cur_buf->payload()[i]  = validateFloat(static_cast<float>(payload[i]));
            }
            break;
            case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_INT:
            {
                std::int32_t * payload = reinterpret_cast<std::int32_t *>(buf);
                cur_buf->payload()[i]  = validateFloat(static_cast<float>(payload[i]));
            }
            break;
            default:
                break;
            }

            return true;
        }
    }

    // перекладемо дані якшо вони вже в форматі float
    std::copy(buf, buf + cur_buf->payloadSize(), cur_buf->payload());

    return true;
}

void DataSourceFrameProcessor::putNewFrame(std::shared_ptr<DataSourceBufferInterface> & buffer, const int & updated_size)
{
    std::lock_guard<std::mutex> lock(m_process_mutex);

    if (m_src_ready_buffer >= static_cast<int>(MAX_PROCESSING_BUF_NUM) - 1)
    {
        m_src_ready_buffer = -1;

        ++m_active_buffer;

        // міняєм буфер
        if (m_active_buffer >= BUFERIZATION_NUM)
            m_active_buffer = 0;


        m_can_validate = true;
    }

    // Готовий буфер для запису
    ++m_src_ready_buffer;

    // Обміняємо буфери для обробки
    m_source_buffer[m_active_buffer][m_src_ready_buffer].swap(buffer);

    // розмір не відповідає необхідному
    if (updated_size != frameSize())
    {
        ++m_bad_frames;
    }
}

} // namespace DATA_SOURCE_TASK
