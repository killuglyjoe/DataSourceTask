#include "DataSourceFrameProcessor.h"

namespace DATA_SOURCE_TASK
{

DataSourceFrameProcessor::DataSourceFrameProcessor(const int & frame_size):
    m_frame_size {frame_size},
    m_packets_loss {0},
    m_stream_broken {0},
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

    const int max_total_elements = m_source_buffer[0][0]->totalElements();
    const int float_frame_size   = FRAME_HEADER_SIZE + max_total_elements * sizeof(float);

    // float буфери. К-сть елементів максимальна,
    // тому будемо повертати реальну к-сть оброблених відліків з джерела.
    for (std::size_t i = 0; i < MAX_PROCESSING_BUF_NUM; i++)
    {
        m_buffer.push_back(std::make_shared<DataSourceBuffer<float>>(float_frame_size));
    }

    m_is_process_active = true;
    m_process_thread    = std::thread(&DataSourceFrameProcessor::frameProcess, this);
}

DataSourceFrameProcessor::~DataSourceFrameProcessor()
{
    m_is_process_active = false;

    if (m_process_thread.joinable())
        m_process_thread.join();
}

void DataSourceFrameProcessor::frameProcess()
{
    static Timer timer;

    while (m_is_process_active)
    {
        if (m_can_validate)
        {
            timer.reset();

            for (std::size_t idx = 0; idx < MAX_PROCESSING_BUF_NUM; ++idx)
            {
                int total_elements = 0;

                // поточний буфер оновлюється, тому беремо попередній.
                int ready_buffer = m_active_buffer - 1;

                if (ready_buffer < 0)
                {
                    ready_buffer = BUFERIZATION_NUM - 1;
                }

                total_elements = validateFrame(m_source_buffer[ready_buffer][idx]);

                if (total_elements)
                {
                    // Перевіримо ІД джерела і виокремимо для запису в файл
                    const int source_id = static_cast<int>(m_buffer[m_flt_ready_buffer]->frame()->source_id);

                    const auto & it = m_data_source_frame_recorders.find(source_id);

                    if (it != m_data_source_frame_recorders.end())
                    {
                        // реєстрація блоків даних
                        it->second->putNewFrame(m_buffer[m_flt_ready_buffer], total_elements);
                    }
                    else
                    {
                        // Треба заміряти пам'ять, треба знати коли зупинитись
                        m_data_source_frame_recorders[source_id] = std::make_shared<DataSourceFrameRecorder>(
                            "record_" + std::to_string(source_id), total_elements);
                    }
                }
            }

            m_can_validate = false;
            m_elapsed      = timer.elapsed();

            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int DataSourceFrameProcessor::validateFrame(const std::shared_ptr<DataSourceBufferInterface> & buffer)
{
    std::lock_guard<std::mutex> lock(m_process_mutex);

    uint32_t total_elements = buffer->payloadSize() / FLOAT_SIZE;

    frame * frm = buffer->frame();
    char * buf  = buffer->payload();

    // розбираємось з лічильком кадру
    static int cur_frm_counter = -1;

    if (cur_frm_counter == -1)
    {
        cur_frm_counter = frm->frame_counter;
    }
    else
    {
        // лічільник кадрів
        const int delta = frm->frame_counter - cur_frm_counter;

        if ((delta > 1) && (delta < UINT16_MAX))
        {
            m_packets_loss += frm->frame_counter - cur_frm_counter - 1;
        }
    }

    // Запам'ятовуємо лічильник.
    cur_frm_counter = frm->frame_counter;

    // сформуємо float масиви
    if (m_flt_ready_buffer >= static_cast<int>(MAX_PROCESSING_BUF_NUM) - 1)
        m_flt_ready_buffer = -1;

    // Поточний кадр для перетворення в float
    ++m_flt_ready_buffer;

    DataSourceBuffer<float> * cur_buf = m_buffer[m_flt_ready_buffer].get();

    // оновимо заголовок
    std::copy(frm, frm + FRAME_HEADER_SIZE, cur_buf->frame());

    // - реалізувати максимально обчислювально ефективне перетворення усіх даних
    // до єдиного типу 32 bit IEEE 754 float та приведення до діапазону +/-1.0;
    if (frm->payload_type != PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT)
    {
        switch (frm->payload_type)
        {
        case PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT:
        {
            std::uint8_t * payload = reinterpret_cast<std::uint8_t *>(buf);

            total_elements = buffer->payloadSize() / UINT8_SIZE;

            for (std::uint32_t i = 0; i < total_elements; ++i)
            {
                cur_buf->payload()[i] = static_cast<float>(payload[i]);
            }
        }
        break;

        case PAYLOAD_TYPE::PAYLOAD_TYPE_16_BIT_INT:
        {
            std::int16_t * payload = reinterpret_cast<std::int16_t *>(buf);

            total_elements = buffer->payloadSize() / INT16_SIZE;

            for (std::uint32_t i = 0; i < total_elements; ++i)
            {
                cur_buf->payload()[i] = static_cast<float>(payload[i]);
            }
        }
        break;

        case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_INT:
        {
            std::int32_t * payload = reinterpret_cast<std::int32_t *>(buf);

            total_elements = buffer->payloadSize() / INT32_SIZE;

            for (std::uint32_t i = 0; i < total_elements; ++i)
            {
                cur_buf->payload()[i] = static_cast<float>(payload[i]);
            }
        }
        break;

        default:
            break;
        }

        return total_elements;
    }

    // перекладемо дані якшо вони вже в форматі float
    memcpy(cur_buf->payload(), buf, buffer->payloadSize());

    return total_elements;
}

void DataSourceFrameProcessor::putNewFrame(std::shared_ptr<DataSourceBufferInterface> & frame, int updated_size)
{
    std::lock_guard<std::mutex> lock(m_process_mutex);

    if (!frame.get())
        return;

    // Заповнюємо буфери з масивами кадрів
    if (m_src_ready_buffer >= static_cast<int>(MAX_PROCESSING_BUF_NUM) - 1)
    {
        m_src_ready_buffer = -1;

        ++m_active_buffer;

        // міняєм буфер
        if (m_active_buffer >= BUFERIZATION_NUM)
            m_active_buffer = 0;

        // сигналізуємо про готовність масивів даних для обробки
        m_can_validate = true;
    }

    // Новий кадр для масиву даних
    ++m_src_ready_buffer;

    const PAYLOAD_TYPE p_type = frame->frame()->payload_type;

    // Обміняємо кадр для обробки
    m_source_buffer[m_active_buffer][m_src_ready_buffer].swap(frame);

    // розмір не відповідає необхідному.
    if (updated_size != frameSize())
    {
        ++m_bad_frames;
    }

    // перевірка цілісності даних. розмір даних має бути кратним типу даних
    if (updated_size > static_cast<int>(FRAME_HEADER_SIZE))
    {
        int recieved_payload_size = updated_size - FRAME_HEADER_SIZE;
        int payload_size          = 1;

        switch (p_type)
        {
        case PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT:
            return;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_16_BIT_INT:
            payload_size = INT16_SIZE;
            break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_INT:
            payload_size = INT32_SIZE;
            break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT:
            payload_size = FLOAT_SIZE;
            break;
        default:
            break;
        }

        if (recieved_payload_size % payload_size != 0)
        {
            ++m_stream_broken;
        }
    }
    else
    {
        ++m_stream_broken;
    }
}

double DataSourceFrameProcessor::saveFrameElapsed()
{
    double average_elapsed;

    average_elapsed = 0;

    for (const auto & recorder : m_data_source_frame_recorders)
    {
        average_elapsed += recorder.second->elapsed();
    }

    int num_toatal = m_data_source_frame_recorders.size();

    if (num_toatal)
        average_elapsed /= num_toatal;

    return average_elapsed;
}

} // namespace DATA_SOURCE_TASK
