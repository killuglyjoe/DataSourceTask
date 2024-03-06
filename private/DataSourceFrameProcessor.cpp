#include "DataSourceFrameProcessor.h"
#include "DataSourceController.h"
#include <future>
#include <iostream>
#include <ostream>
#include <thread>

namespace DATA_SOURCE_TASK
{

std::thread process_thread;
static std::atomic<bool> is_process_active {true};

DataSourceFrameProcessor::DataSourceFrameProcessor(const int & frame_size, const int & num_elements):
    m_frame_size {frame_size}, m_packets_loss {0}, m_need_validate {false}, m_ready_buffer {-1}
{
    for (std::size_t i = 0; i < MAX_READ_BUF_NUM; i++)
        m_buffer[i] = std::make_shared<DataSourceBuffer<float>>(num_elements);

    m_data_source_recorder = std::make_unique<DataSourceFrameRecorder>("record", num_elements);

    process_thread = std::thread(&DataSourceFrameProcessor::frameProcess, this);
}

DataSourceFrameProcessor::~DataSourceFrameProcessor() { is_process_active = false; }

std::future<void> future_result;
void DataSourceFrameProcessor::frameProcess()
{
    Timer timer;
    while (is_process_active)
    {
        if (m_need_validate)
        {
            timer.reset();
            if (validateFrame(m_buffer_to_process, m_req_size))
            {
                // реєстрація блоків даних
                m_data_source_recorder->putNewFrame(curProcessedFrame());
            }

            m_elapsed = timer.elapsed();

            m_need_validate = false;
        }
    }
}

bool DataSourceFrameProcessor::validateFrame(DataSourceBufferInterface * buffer, const int & updated_size)
{
    std::lock_guard<std::mutex> lock(m_process_mutex);

    frame * frm    = buffer->frame();
    char * payload = buffer->payload();

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

    // розмір не відповідає необхідному - втрачений кадр?
    if (updated_size != frameSize())
    {
        ++m_packets_loss;
    }

    // Готовий буфер для запису
    ++m_ready_buffer;

    if (m_ready_buffer >= 2)
        m_ready_buffer = 0;

    DataSourceBuffer<float> * cur_buf = m_buffer[m_ready_buffer].get();

    // оновимо заголовок
    std::copy(frm, frm + sizeof(struct frame), cur_buf->frame());

    // - реалізувати максимально обчислювально ефективне перетворення усіх даних
    // до єдиного типу 32 bit IEEE 754 float та приведення до діапазону +/-1.0;
    if (frm->payload_type != PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT)
    {
        // CPU
        for (int i = 0; i < cur_buf->payloadSize(); ++i)
        {
            cur_buf->payload()[i] = static_cast<float>(payload[i]);
        }

        return true;
    }

    // перекладемо дані якшо вони вже в форматі float
    std::copy(payload, payload + cur_buf->payloadSize() * cur_buf->typeSize(), cur_buf->payload());

    return true;
}

void DataSourceFrameProcessor::putNewFrame(const std::shared_ptr<DataSourceBufferInterface> & buffer,
                                           const int & updated_size)
{
    m_buffer_to_process = buffer.get();
    m_req_size          = updated_size;
    m_need_validate     = true;
}

} // namespace DATA_SOURCE_TASK
