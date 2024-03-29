#include "DataSourceFrameRecorder.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

namespace DATA_SOURCE_TASK
{

// Функція вертає найближче значення числа степеня 2 ( наприклад: для 10 -> 16 = 2^4)
size_t nearestPowerOfTwo(const size_t & n)
{
    if (n <= 1)
        return 1;

    return static_cast<size_t>(std::pow(2, std::ceil(std::log2(n))));
}

DataSourceFrameRecorder::DataSourceFrameRecorder(const std::string & record_name,
                                                 const int & num_elements):
    m_record_name {record_name},
    m_need_record {false}
{
    m_buffer_size = nearestPowerOfTwo(num_elements * RECORD_SIZE);

    // Буфери для запису розміром кратним степеня двійки
    for (std::size_t i = 0; i < MAX_REC_BUF_NUM; ++i)
    {
        struct record_buffer * buf = &m_frame_record[i];
        buf->record_buffer.resize(m_buffer_size);
        buf->available_size = m_buffer_size;
        buf->id             = i + 1;
    }

    // Дані для запису в файл
    m_record_buffer.resize(m_buffer_size);

    // асинхронний потік запису в файл
    m_record_to_file = std::thread(&DataSourceFrameRecorder::recordBlock, this);
}

DataSourceFrameRecorder::~DataSourceFrameRecorder()
{
    m_is_can_record_active = false;

    if (m_record_to_file.joinable())
        m_record_to_file.join();
}

void DataSourceFrameRecorder::recordBlock()
{
    m_is_can_record_active = true;

    Timer timer;
    while (m_is_can_record_active)
    {
        if (m_need_record)
        {
            timer.reset();

            // Будемо просто перезаписувати поточний файл.
            std::ofstream source_file(m_record_name, std::ios::out | std::ios::binary);

            if (!source_file)
                continue;

            char * wbuf  = reinterpret_cast<char *>(m_record_buffer.data());
            int buz_size = m_record_buffer.size()  * FLOAT_SIZE;

            source_file.write(wbuf, buz_size);

            m_need_record = false;

            if (source_file.fail())
            {
                std::ios_base::iostate state = source_file.rdstate();

                if (state & std::ios_base::eofbit)
                {
                    std::cout << "DataSourceFrameRecorder: End of file reached." << std::endl;
                }
                if (state & std::ios_base::failbit)
                {
                    std::cout << "DataSourceFrameRecorder: Non-fatal I/O error occurred." << std::endl;
                }
                if (state & std::ios_base::badbit)
                {
                    std::cout << "DataSourceFrameRecorder: Fatal I/O error occurred." << std::endl;
                }
            }

            m_elapsed = timer.elapsed();

            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void DataSourceFrameRecorder::putNewFrame(const std::shared_ptr<DataSourceBuffer<float>> & frame,
                                          const int & total_elements)
{
    std::lock_guard<std::mutex> lock(m_buf_lock);

    if (!frame.get())
        return;

    // Реальний розмір оброблених даних
    std::size_t av_in_data = total_elements * FLOAT_SIZE;

    // Заповнимо масиви під запис
    for (std::uint8_t i = 0; i < MAX_REC_BUF_NUM; ++i)
    {
        struct record_buffer * buf = &m_frame_record[i];

        // перевіримо чи буфер не повний і чи є що записати ще
        if (!buf->is_full && av_in_data > 0)
        {
            // вільне місце в буфері
            std::size_t num_data_store = buf->available_size;

            if (av_in_data < num_data_store)
            {
                num_data_store = av_in_data;
            }
            else
            {
                num_data_store = buf->available_size;
            }

            memcpy(buf->record_buffer.data() + buf->pos, frame->payload(), num_data_store);

            buf->pos += num_data_store;                 // зміщуємо позицію в буфері для наступного дозапису
            buf->available_size -= num_data_store;      // оновлюємо розмір вільного місця
            buf->is_full = (buf->pos >= m_buffer_size); // ставим прапорець заповненості буферу

            if (buf->is_full)
            {
                // Обміняємо буфери для запису
                m_record_buffer.swap(buf->record_buffer);

                // дозволяємо запис в файл
                m_need_record = true;
            }

            // зменшуємо розмір даних для копіювання в буфери запису
            av_in_data -= num_data_store;
        }
        else
        {
            // вивільняємо дані
            buf->is_full        = false;
            buf->available_size = buf->record_buffer.size();
            buf->pos            = 0;
        }
    }
}

} // namespace DATA_SOURCE_TASK
